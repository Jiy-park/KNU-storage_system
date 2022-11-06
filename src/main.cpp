#include"ftl.h"
using namespace std;

//MEGABYTE_SIZE = sizeof(BLOCK) * BLOCK_SIZE * num of block in memory 

int main(){
    FTL bast;
    bast.init();
    // cout<<sizeof(FTL);
    bast.test();
    cout<<"BAST::total_input_count : "<<bast.input_count<<"\n";
    cout<<"BAST::read_count : "<<bast.read_count<<"\n";
    cout<<"BAST::write_count : "<<bast.write_count<<"\n";
    cout<<"BAST::erase_count : "<<bast.erase_count<<"\n";
    cout<<"BAST::merge_operation_count : "<<bast.merge_count<<"\n";
    cout<<"BAST::switch_operation_count : "<<bast.switch_count<<"\n";
    cout<<"\n";
    return 0;
}