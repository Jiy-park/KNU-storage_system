#ifndef __FLASH_MEMORY_H__
#define __FLASH_MEMORY_H__

#include<iostream>
#include"setting.h"
#include<fstream>
using namespace std;


class FLASH_MEMORY{
public:
    bool init();
    bool flash_write(const int block_index, const int sector_index, const char data[]);
    bool flash_read(const int block_index, const int sector_index);
    bool flash_erase(const int block_index);
    
    int get_memory_size() const { return flash_memory_size; }
    bool get_is_using(const int block_index, const int sector_index) const { return flash_memory[block_index].sector[sector_index].is_using; }
    void set_is_using(const int block_index, const int sector_index, bool option) const { flash_memory[block_index].sector[sector_index].is_using = option; }
    BLOCK& get_one_block(const int block_index) const { return flash_memory[block_index]; }
    int get_block_wear_level(const int block_index) const { return flash_memory[block_index].wear_level; }

    FLASH_MEMORY(){};
    ~FLASH_MEMORY(){
        cout<<"FLASH_MEMORY::called ~FLASH_MEMORY()\n";
        delete[] flash_memory;
    };
    //////////for test
    void print_memory(const int from, const int to)const;
private:
    BLOCK* flash_memory = nullptr;
    int flash_memory_size = 0;
};

#endif