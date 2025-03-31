from collections import deque


# TLB = 16 entries total
# only FIFO -> Queue
# purpose of a TLB is for efficiency
# MOST RECENTLY page -> frame or most likely to be used again
# so TLB keeps most recently used

# soft miss -> didnt hit in the TLB, but hit in the page Table

class TLB:
    def __init__(self):
        self.size = 16
        self.tlb = deque()

    def get_frame_number(self, page_number):
        for i, entry in enumerate(self.tlb):
            if entry['page_number'] == page_number:
                # fifo
                entry = self.tlb[i]
                self.tlb.rotate(-i)
                self.tlb.append(self.tlb.popleft())
                return entry['frame_number']
        return None

    def update_tlb(self, page_number, frame_number):
        # if entry exists, update
        self.tlb = deque(entry for entry in self.tlb if entry['page_number'] != page_number)

        # tlb is FULL -> FIFO
        if len(self.tlb) >= self.size:
            self.tlb.popleft()

        # check if entry already exists in page table (MISS) load it back in
        # if its not in page table -> PAGE FAULT (load page from backing store)
        # then update page table and TLB

        # add new entry
        self.tlb.append({'page_number': page_number, 'frame_number': frame_number})
