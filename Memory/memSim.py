# TLB only uses FIFO
# Physical memory could use FIFO, LRU, or OPT
import argparse
from TLB import TLB
from pageTable import pageTable
from physcialMemory import physicalMemory


class memSim:
    def __init__(self, FRAMES, PRA, backing_store='BACKING_STORE.bin'):
        self.tlb = TLB()
        self.page_table = pageTable()
        self.physical_memory = physicalMemory(input_frames=FRAMES)
        self.PRA = PRA
        self.backing_store = backing_store
        self.translated_addresses = 0
        self.total_page_faults = 0
        self.tlb_hits = 0
        self.tlb_misses = 0
        self.future_addresses = []

    def start(self, input_file):
        with open(input_file, 'r') as file:
            self.future_addresses = [int(line.strip()) for line in file]
        self.physical_memory.future(self.future_addresses)

        # go through each logical address and do the process
        for address_index, address in enumerate(self.future_addresses):
            self.translated_addresses += 1
            print(self.process_logical_address(address, address_index))
        self.stats()

    def process_logical_address(self, logical_address, address_index):
        page_number = logical_address // 256
        offset = logical_address % 256

        # tlb: (k, v) = (page_number, frame_number)
        frame_number = self.tlb.get_frame_number(page_number)
        # if frame number is NOT in TLB
        if frame_number is None:
            # check page table.
            self.tlb_misses += 1
            frame_number = self.page_table.get_frame_number(page_number)
            # If not in page table -> page_fault()
            if frame_number is None:
                frame_number = self.page_fault(page_number, address_index)
                self.total_page_faults += 1
            self.tlb.update_tlb(page_number, frame_number)
        else:
            self.tlb_hits += 1

        # physical_memory: (k, v) = (frame_number, data)
        data = self.physical_memory.get_frame_data(frame_number)
        data_value = int.from_bytes(data[offset:offset + 1], byteorder='little', signed=True)
        page_content = ''.join(f"{byte:02X}" for byte in data) if data else 'No Data Available'

        return f"{logical_address}, {data_value}, {frame_number}, {page_content}"

    def page_fault(self, page_number, address_index):
        try:
            with open(self.backing_store, 'rb') as store:
                # find page number in backing_store
                store.seek(page_number * 256)
                # reads the 256 byte block of data corresponding to page_number
                page_data = store.read(256)
        except Exception as e:
            print(f"Error: page_fault")
            return None

        # find first free frame in physical memory
        frame_number = self.physical_memory.get_emtpy_frame_or_replace(self.PRA, current_address_index=address_index)
        # page fault -> update physical memory, page table, tlb
        self.physical_memory.set_frame_data(frame_number, page_data)
        self.page_table.set_entry(page_number, frame_number, loaded_bit=True)
        self.tlb.update_tlb(page_number, frame_number)

        return frame_number

    def stats(self):
        page_fault_rate = self.total_page_faults / self.translated_addresses
        tlb_hit_rate = self.tlb_hits / self.translated_addresses
        print(f"Number of Translated Addresses = {self.translated_addresses}")
        print(f"Page Faults = {self.total_page_faults}")
        print(f"Page Fault Rate = {page_fault_rate:.3f}")
        print(f"TLB Hits = {self.tlb_hits}")
        print(f"TLB Misses = {self.tlb_misses}")
        print(f"TLB Hit Rate = {tlb_hit_rate:.3f}")


def main():
    # https://docs.python.org/3/library/argparse.html#nargs
    parser = argparse.ArgumentParser(
        prog='memSim',
        description='Memory Simulator'
    )
    parser.add_argument('FILE')
    parser.add_argument('FRAMES', type=int, nargs='?', default=256)
    parser.add_argument('PRA', nargs='?', default='fifo', choices=['fifo', 'lru', 'opt'])
    args = parser.parse_args()

    memSim(FRAMES=args.FRAMES, PRA=args.PRA).start(args.FILE)


if __name__ == "__main__":
    main()
