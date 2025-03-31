# page table entries = 2^8
# page size = 256 Bytes
# dictionary

# need set, get
class pageTable:
    def __init__(self):
        self.page_table = {}
        self.size = 256

    def set_entry(self, page_number, frame_number, loaded_bit=True):
        if page_number in self.page_table:
            current_entry = self.page_table[page_number]
            current_entry['frame_number'] = frame_number
            current_entry['loaded_bit'] = loaded_bit
        else:
            # update
            self.page_table[page_number] = {'frame_number': frame_number, 'loaded_bit': loaded_bit}

    def get_frame_number(self, page):
        current_entry = self.page_table.get(page)
        # if entry exists and bit is set return FRAME
        if current_entry and current_entry['loaded_bit']:
            return current_entry['frame_number']
        return None
