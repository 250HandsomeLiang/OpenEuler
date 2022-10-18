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
#define NBUCKETS 13
void moveB(int index,struct buf *b);
struct {
  struct spinlock lock[NBUCKETS];
  struct buf buf[NBUF];

  // Linked list of all buffers, through prev/next.
  // Sorted by how recently the buffer was used.
  // head.next is most recent, head.prev is least.
  struct buf head[NBUCKETS];
} bcache;


//init block
void
binit(void)
{
  struct buf *b;
  for(int i=0;i<NBUCKETS;i++){
      initlock(&bcache.lock[i], "bcache");
  }
  int index=0;
  // Create linked list of buffers
  for(int i=0;i<NBUCKETS;i++){
    bcache.head[i].prev = &bcache.head[i];
    bcache.head[i].next = &bcache.head[i];
  }

  for(b = bcache.buf; b < bcache.buf+NBUF; b++){
    index=b->blockno%NBUCKETS;
    b->next = bcache.head[index].next;
    b->prev = &bcache.head[index];
    initsleeplock(&b->lock, "buffer");
    bcache.head[index].next->prev = b;
    bcache.head[index].next = b;
  }
}

// Look through buffer cache for block on device dev.
// If not found, allocate a buffer.
// In either case, return locked buffer.
static struct buf*
bget(uint dev, uint blockno)
{
  struct buf *b;
  int index=blockno%NBUCKETS;
  acquire(&bcache.lock[index]);

  // Is the block already cached?
  for(b = bcache.head[index].next; b != &bcache.head[index]; b = b->next){
    if(b->dev == dev && b->blockno == blockno){
      b->refcnt++;
      release(&bcache.lock[index]);
      //when a process is using this buffer,the other process can't use it
      acquiresleep(&b->lock);
      return b;
    }
  }

  // Not cached.
  // Recycle the least recently used (LRU) unused buffer.
  // find in own hashMap first
  for(b = bcache.head[index].prev; b != &bcache.head[index]; b = b->prev){
    if(b->refcnt == 0) {
      b->dev = dev;
      b->blockno = blockno;
      b->valid = 0;
      b->refcnt = 1;
      release(&bcache.lock[index]);
      acquiresleep(&b->lock);
      return b;
    }
  }
  // find in other hashMap
  // from right to left
  for(int i=index+1;i<NBUCKETS;i++){
    acquire(&bcache.lock[i]);
    for(b = bcache.head[i].prev; b != &bcache.head[i]; b = b->prev){
      if(b->refcnt == 0) {
        b->dev = dev;
        b->blockno = blockno;
        b->valid = 0;
        b->refcnt = 1;
        //move b to bcache.head[index]
        moveB(index,b);
        release(&bcache.lock[i]);
        release(&bcache.lock[index]);
        acquiresleep(&b->lock);
        return b;
      }
    }
    release(&bcache.lock[i]); 
  }

  for(int i=0;i<index;i++){
    acquire(&bcache.lock[i]);
    for(b = bcache.head[i].prev; b != &bcache.head[i]; b = b->prev){
      if(b->refcnt == 0) {
        b->dev = dev;
        b->blockno = blockno;
        b->valid = 0;
        b->refcnt = 1;
        moveB(index,b);
        release(&bcache.lock[i]);
        release(&bcache.lock[index]);
        acquiresleep(&b->lock);
        return b;
      }
    }
    release(&bcache.lock[i]); 
  }
  panic("bget: no buffers");
}

// Return a locked buf with the contents of the indicated block.
struct buf*
bread(uint dev, uint blockno)
{
  struct buf *b;

  b = bget(dev, blockno);
  if(!b->valid) {
    //read data from RAM
    virtio_disk_rw(b, 0);
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
  virtio_disk_rw(b, 1);
}

// Release a locked buffer.
// Move to the head of the most-recently-used list.
void
brelse(struct buf *b)
{
  if(!holdingsleep(&b->lock))
    panic("brelse");

  releasesleep(&b->lock);
  int index=b->blockno%NBUCKETS;
  acquire(&bcache.lock[index]);
  b->refcnt--;
  if (b->refcnt == 0) {
    // no one is waiting for it.
    b->next->prev = b->prev;
    b->prev->next = b->next;
    b->next = bcache.head[index].next;
    b->prev = &bcache.head[index];
    bcache.head[index].next->prev = b;
    bcache.head[index].next = b;
  }
  
  release(&bcache.lock[index]);
}

void
bpin(struct buf *b) {
  int index=b->blockno%NBUCKETS;
  acquire(&bcache.lock[index]);
  b->refcnt++;
  release(&bcache.lock[index]);
}

void
bunpin(struct buf *b) {
  int index=b->blockno%NBUCKETS;
  acquire(&bcache.lock[index]);
  b->refcnt--;
  release(&bcache.lock[index]);
}
// move free buffer to bcache.head[i]
void moveB(int index,struct buf *b )
{
    b->next->prev = b->prev;
    b->prev->next = b->next;
    b->next = bcache.head[index].next;
    b->prev = &bcache.head[index];
    bcache.head[index].next->prev = b;
    bcache.head[index].next = b;
}


