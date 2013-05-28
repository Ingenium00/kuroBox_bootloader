#ifndef HELPER_H_
#define HELPER_H_

struct FlashSector {
	bool_t protected;
	bool_t erased;
};

void flash_init(struct FlashSector * flash);
void flash_finish(struct FlashSector* flash);
int flash_write(struct FlashSector* flash, uint32_t address, const uint8_t * buffer, int count);

#endif /* HELPER_H_ */
