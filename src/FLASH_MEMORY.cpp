#include"flash_memory.h"


bool FLASH_MEMORY::init(){
    cout<<"FLASH_MEMORY::init :: enter falsh_memory size(mb) : ";
    //for_test
    int input_size = 1;
    //cin>>input_size;
    flash_memory_size = (int)(input_size * MEGABYTE_SIZE / sizeof(BLOCK));
    flash_memory = new BLOCK[flash_memory_size];
    if(flash_memory == nullptr) { 
        cout<<"FLASH_MEMORY::init :: fail to create flash_memory\n";
        return false;
    }
    cout<<"FLASH_MEMORY::init :: created flash_memory, size : "<<flash_memory_size<<"\n";
    return true;
};


bool FLASH_MEMORY::flash_write(const int block_index, const int sector_index, const char data[]){
    if(block_index > flash_memory_size || block_index < 0 || sector_index > BLOCK_SIZE || sector_index < 0 || sizeof(data) >= SECTOR_SIZE){
        cout<<"FLASH_MEMORY::flash_write :: try to access out of range block_index : <<"<<block_index<<", sector_index : "<<sector_index<<"\n";
        return false;
    }
    if(flash_memory[block_index].sector[sector_index].is_using == true
        || flash_memory[block_index].sector[sector_index].data[0] != '\0') { 
        cout<<"FLASH_MEMORY::flash_write( "<<block_index<<", "<<sector_index<<" ) :: already using now\n";
        return false;
    }
    strcpy_s(flash_memory[block_index].sector[sector_index].data, data);
    cout<<"FLASH_MEMORY::flash_write( "<<block_index<<", "<<sector_index<<" ) :: "<<flash_memory[block_index].sector[sector_index].data<<"\n";
    flash_memory[block_index].sector[sector_index].is_using = true;


    return true;
};

bool FLASH_MEMORY::flash_read(const int block_index, const int sector_index){
    if(block_index > flash_memory_size || block_index < 0 || sector_index > BLOCK_SIZE || sector_index < 0){
        cout<<"FLASH_MEMORY::flash_erase :: try to access out of range block_index : <<"<<block_index<<", sector_index : "<<sector_index<<"\n";
        return false;
    }
    if(flash_memory[block_index].sector[sector_index].is_using == true){
        cout<<"FLASH_MEMORY::flash_read( "<<block_index<<", "<<sector_index<<" ) :: "<<flash_memory[block_index].sector[sector_index].data<<"\n";
        return true;
    }
    if(flash_memory[block_index].sector[sector_index].data[0] != '\0'){
        cout<<"FLASH_MEMORY::flash_read( "<<block_index<<", "<<sector_index<<" ) :: data was updated\n";
        return false;
    }
    cout<<"FLASH_MEMORY::flash_read( "<<block_index<<", "<<sector_index<<" ) :: no data\n";
    return true;
};

bool FLASH_MEMORY::flash_erase(const int block_index){
    if(block_index < 0 || block_index > flash_memory_size) {
        cout<<"FLASH_MEMORY::flash_erase :: try to access out of range block_index : <<"<<block_index<<"\n";
        return false;
    }
    for(int i = 0; i < BLOCK_SIZE; i++){
        strcpy_s(flash_memory[block_index].sector[i].data, "");
        flash_memory[block_index].sector[i].is_using = false;
    }
    flash_memory[block_index].wear_level++;
    cout<<"FLASH_MEMORY::flash_erase( "<<block_index<<" ) :: complete\n";
    return true;
};

void FLASH_MEMORY::print_memory(const int from, const int to)const{
    for(int i = from; i < to; i++){
        cout<<"block_num : "<<i<<" index data using\n";
        for(int j = 0; j < BLOCK_SIZE; j++){
            cout<<"sector_num : "<<j<<" "<<flash_memory[i].sector[j].data<<"\t"<<flash_memory[i].sector[j].is_using<<"\n";
        }
        cout<<"\n\n\n";
    }
};