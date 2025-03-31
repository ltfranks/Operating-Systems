class physicalMemory:
    def __init__(self, input_frames):
        self.frames = [None] * input_frames
        self.frame_size = 256
        self.frame_history = []
        self.frame_last_used_time = {}
        self.current_time = 0
        self.future_frames = []

    def set_frame_data(self, frame_number, data):
        self.frames[frame_number] = data
        # update LRU time
        self.frame_last_used_time[frame_number] = self.current_time
        self.current_time += 1
        # update fifo history
        if frame_number not in self.frame_history:
            self.frame_history.append(frame_number)

    def get_frame_data(self, frame_number):
        # if frame is fetched again, update its time slot
        self.frame_last_used_time[frame_number] = self.current_time
        self.current_time += 1
        return self.frames[frame_number]

    def get_emtpy_frame_or_replace(self, PRA, current_address_index=None):
        if None in self.frames:
            return self.frames.index(None)  # returns first empty frame

        if PRA == 'fifo':
            oldest_frame = self.frame_history.pop(0)
            self.frame_history.append(oldest_frame)
            return oldest_frame
        elif PRA == 'lru':
            lru_frame = min(self.frame_last_used_time, key=self.frame_last_used_time.get)
            self.frame_last_used_time.pop(lru_frame)
            return lru_frame
        elif PRA == 'opt' and current_address_index is not None:
            return  self.opt_algorithm(current_address_index)

    def future(self, addresses):
        self.future_frames = addresses

    def opt_algorithm(self, current_address):
        future_use = {frame: float('inf') for frame in range(len(self.frames))}
        for future_index in range(current_address+1, len(self.future_frames)):
            page_number = self.future_frames[future_index] // 256
            for frame_number, data in enumerate(self.frames):
                if data is not None:
                    frame_page_number = self.future_frames.index(int.from_bytes(data[:2], byteorder='little'))
                    if frame_page_number == page_number and future_use[frame_number] == float('inf'):
                        future_use[frame_number] = future_index
                        break
        return max(future_use, key=future_use.get())

