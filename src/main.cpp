#include"ftl.h"
using namespace std;

//MEGABYTE_SIZE = sizeof(BLOCK) * BLOCK_SIZE * num of block in memory 

int main(){
    FTL bast;
    bast.init();
    // cout<<sizeof(FTL);
    bast.test();
    cout<<bast.merge_count<<"\n";
    cout<<bast.switch_count<<"\n";
    return 0;
}