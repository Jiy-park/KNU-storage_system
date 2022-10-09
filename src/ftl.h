#ifndef __FTL_H__
#define __FTL_H__

#include"flash_memory.h"
#include<queue>


class FTL{
public:
    int k = 1;
    bool init();
    bool FTL_write(const int index, const char data[]);
    bool FTL_read(const int index);

    FTL(){ flash_memory.init(); };
    ~FTL(){
        cout<<"FTL::called ~FTL()\n";
        delete[] block_mapping_table;
        delete[] sector_mapping_table;
    };
    /////////for test
    void test();
    void test2();

    //for_test
    int switch_count = 0;
    int merge_count = 0;
    int write_count = 0;
    int read_count = 0;
    int erase_count = 0;
private:
    bool merge_operation(const int lbn, const int pbn, const int log_pbn);
    bool switch_operation(const int lbn, const int pbn, const int log_pbn);
    bool init_block_Q(const int flash_memory_size);
    bool cmp_Q_size(const int wear_level, const int index);
    FLASH_MEMORY flash_memory;
    priority_queue<BW_pair> log_block_Q;
    priority_queue<BW_pair> data_block_Q;
    BMP* block_mapping_table = nullptr;
    SMP* sector_mapping_table = nullptr;
    int data_block_ratio = 0;
};

#endif