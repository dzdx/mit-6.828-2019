// Buffer cache.
//
// The buffer cache is a linked list of buf structures holding
// cached copies of disk block contents.  Caching disk blocks
// in memory reduces the number of disk reads and also provides
// a synchronization point for disk blocks used by multiple processes.
//
// Interface:
// * To get a buffer for a particular disk block, call bread.
// * After changing buffer data, call bwrite to write it to disk.
// * When done with the buffer, call brelse.
// * Do not use the buffer after calling brelse.
// * Only one process at a time can use a buffer,
//     so do not keep them longer than necessary.


#include "types.h"
#include "param.h"
#include "spinlock.h"
#include "sleeplock.h"
#include "riscv.h"
#include "defs.h"
#include "fs.h"
#include "buf.h"

#define  NBUCKET 13

struct bucket{
  struct spinlock lock;
  struct buf head;
};

struct {
  struct spinlock lock;
  struct buf buf[NBUF];
  struct bucket buckets[NBUCKET];

  // Linked list of all buffers, through prev/next.
  // head.next is most recently used.
} bcache;

void
binit(void)
{
  struct buf *b;

  initlock(&bcache.lock, "bcache");
  for(int i=0;i<NBUCKET;i++){
    initlock(&bcache.buckets[i].lock, "bcache.bucket");
    struct buf *head = &bcache.buckets[i].head;
    head->prev = head;
    head->next = head;
  }
  for(b = bcache.buf; b < bcache.buf+NBUF; b++){
    initsleeplock(&b->lock, "buffer");
    b->next = b;
    b->prev = b;
  }
}

// Look through buffer cache for block on device dev.
// If not found, allocate a buffer.
// In either case, return locked buffer.
static struct buf*
bget(uint dev, uint blockno)
{
  struct buf *b;
  // Is the block already cached?
 struct bucket *bkt =  &bcache.buckets[blockno % NBUCKET];
 acquire(&bkt->lock);
 for(b=bkt->head.next;b != &bkt->head;b = b->next){
   if(b->dev == dev && b->blockno == blockno){
     b->refcnt++;
     release(&bkt->lock);
     acquiresleep(&b->lock);
     return b;
   }
 }
 release(&bkt->lock);
  // Not cached; recycle an unused buffer.
 acquire(&bcache.lock);
 uint oldest_ticks = -1;
 struct buf*unused_buf = 0 ;
 for(b = bcache.buf;b<bcache.buf+NBUF;b++) {
   if (b->refcnt == 0) {
     if (oldest_ticks > b->ticks) {
       oldest_ticks = b->ticks;
       unused_buf = b;
     }
   }
 }

 if(unused_buf){
   if(unused_buf->blockno %NBUCKET != blockno % NBUCKET){
     // remove from old bucket
     struct bucket *oldbkt = &bcache.buckets[unused_buf->blockno%NBUCKET];
     acquire(&oldbkt->lock);
     unused_buf->dev = dev;
     unused_buf->blockno = blockno;
     unused_buf->valid = 0;
     unused_buf->refcnt = 1;

     unused_buf->next->prev = unused_buf->prev;
     unused_buf->prev->next = unused_buf->next;
     release(&oldbkt->lock);
     // add to new bucket;
     acquire(&bkt->lock);

     unused_buf->next = bkt->head.next;
     unused_buf->prev = &bkt->head;
     unused_buf->next->prev = unused_buf;
     unused_buf->prev->next = unused_buf;
     release(&bkt->lock);
   }else{
     acquire(&bkt->lock);
     unused_buf->dev = dev;
     unused_buf->blockno = blockno;
     unused_buf->valid = 0;
     unused_buf->refcnt = 1;
     release(&bkt->lock);
   }
   release(&bcache.lock);
   acquiresleep(&unused_buf->lock);
   return unused_buf;
 }
  release(&bcache.lock);
  panic("bget: no buffers");
}

// Return a locked buf with the contents of the indicated block.
struct buf*
bread(uint dev, uint blockno)
{
  struct buf *b;

  b = bget(dev, blockno);
  if(!b->valid) {
    virtio_disk_rw(b->dev, b, 0);
    b->valid = 1;
  }
  return b;
}

// Write b's contents to disk.  Must be locked.
void
bwrite(struct buf *b)
{
  if(!holdingsleep(&b->lock))
    panic("bwrite");
  virtio_disk_rw(b->dev, b, 1);
}

// Release a locked buffer.
// Move to the head of the MRU list.
void
brelse(struct buf *b)
{
  if(!holdingsleep(&b->lock))
    panic("brelse");

  releasesleep(&b->lock);
  struct bucket *bkt = &bcache.buckets[b->blockno%NBUCKET];
  acquire(&bkt->lock);
  b->refcnt--;
  if (b->refcnt == 0) {
    // no one is waiting for it.
    b->ticks = ticks;
  }
  release(&bkt->lock);
}

void
bpin(struct buf *b) {
  acquire(&bcache.buckets[b->blockno%NBUCKET].lock);
  b->refcnt++;
  release(&bcache.buckets[b->blockno%NBUCKET].lock);
}

void
bunpin(struct buf *b) {
  acquire(&bcache.buckets[b->blockno%NBUCKET].lock);
  b->refcnt--;
  release(&bcache.buckets[b->blockno%NBUCKET].lock);
}


