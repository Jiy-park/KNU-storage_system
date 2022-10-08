#ifndef __SETTING_H__
#define __SETTING_H__


//MEGABYTE_SIZE = sizeof(BLOCK) * BLOCK_SIZE * num of block in memory 
#define MEGABYTE_SIZE 50000
#define SECTOR_SIZE 512
#define BLOCK_SIZE 4
#define LOG_BLOCK_RATIO 0.4


//flash_memory
struct SECTOR{
    char data[SECTOR_SIZE] = "";
    bool is_using = false;
};

struct BLOCK{
    SECTOR block[BLOCK_SIZE] = {};
    int wear_level = 0;  
};


//FTL-bast
typedef struct SECTOR_MAPPING_STRUCTURE{
    int log_block = -1;
    int sequential_checker = -1;
    int sector_mapping[BLOCK_SIZE];
}SMP;

struct BW_pair{
    int wear_level = 0;
    int block_index = 0;
    bool operator<(const BW_pair& a)const {
        if(this->wear_level == a.wear_level) { return this->block_index > a.block_index; }
        return this->wear_level > a.wear_level;
    }
};

typedef struct BLOCK_MAPPING_ELIMENTS{
    int pbn = -1;
    int recently_access_sector = 0;
}BME;

#endif