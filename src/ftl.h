#ifndef __FTL_H__
#define __FTL_H__

#include"flash_memory.h"
#include<queue>
#include<vector>

class FTL{
public:
    bool init();
    bool FTL_write(const int index, const char data[]);
    bool FTL_read(const int index);

    FTL(){ flash_memory.init(); };
    ~FTL(){
        cout<<"FTL::called ~FTL()\n";
        delete[] BMT;
        delete[] log_BMT;
    };

    void test1();
    void test2();
    void test3();
    class FOR_TEST{
    public:
        int input_count = 0;
        int switch_count = 0;
        int merge_count = 0;
        int write_count = 0;
        int write_fail_count = 0;
        int read_count = 0;
        int erase_count = 0;
        int wear_level_check[650] = {0,};


        typedef struct OPERATION_TIME{
            int input_count = 0;
            time_t operation_time;
        }OT;

        time_t start,end;
        vector<OT> write_time;
        vector<OT> read_time;
        vector<OT> erase_time;
        vector<OT> merge_operation_time;
        vector<OT> switch_operation_time;

        void fout_write_time();
        void fout_read_time();
        void fout_erase_time();
        void fout_merge_operation_time();
        void fout_switch_operation_time();
    };
    void fout_wear_level();

    FOR_TEST test;
private:
    bool merge_operation(const int lbn, const int pbn, const int log_pbn);
    bool switch_operation(const int lbn, const int pbn, const int log_pbn);
    bool init_block_Q(const int flash_memory_size);
    bool cmp_Q_size(const int wear_level, const int index);
    FLASH_MEMORY flash_memory;
    priority_queue<BW_pair> log_block_Q;
    priority_queue<BW_pair> data_block_Q;
    BMS* BMT = nullptr;
    LBMS* log_BMT = nullptr;
    int data_block_ratio = 0;
};

#endif