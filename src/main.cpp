#include"ftl.h"
using namespace std;

//MEGABYTE_SIZE = sizeof(BLOCK) * BLOCK_SIZE * num of block in memory 

int main(){
    char start;
    FTL bast;
    bast.init();
    // cout<<sizeof(FTL);
    
    cin>>start;
    
    bast.test3();
    cout<<"BAST::total_input_count : "<<bast.test.input_count<<"\n";
    cout<<"BAST::read_count : "<<bast.test.read_count<<"\n";
    cout<<"BAST::write_count : "<<bast.test.write_count<<"\n";
    cout<<"BAST::write_fail_count : "<<bast.test.write_fail_count<<"\n";
    cout<<"BAST::erase_count : "<<bast.test.erase_count<<"\n";
    cout<<"BAST::merge_operation_count : "<<bast.test.merge_count<<"\n";
    cout<<"BAST::switch_operation_count : "<<bast.test.switch_count<<"\n";
    cout<<"\n";

    bast.test.fout_write_time();
    bast.test.fout_read_time();
    bast.test.fout_erase_time();
    bast.test.fout_merge_operation_time();
    bast.test.fout_switch_operation_time();
    bast.fout_wear_level();
    return 0;
}