#include<iostream>
#include<queue>
#include<fstream>
using namespace std;


//MEGABYTE_SIZE = sizeof(BLOCK) * BLOCK_SIZE * num of block in memory 

#define MEGABYTE_SIZE 50000
#define SECTOR_SIZE 512
#define BLOCK_SIZE 4
#define LOG_BLOCK_RATIO 0.4

struct SECTOR{
    char data[SECTOR_SIZE] = "";
    bool is_using = false;
};

struct BLOCK{
    SECTOR block[BLOCK_SIZE] = {};
    int recently_access_sector = 0;
    int wear_level = 0;  
};

typedef struct SECTOR_MAPPING_STRUCTURE{
    int log_block = -1;
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

class FLASH_MEMORY{
    public:
        bool init();
        bool flash_write(const int block_index, const int sector_index, const char data[]);
        bool flash_read(const int block_index, const int sector_index);
        bool flash_erase(const int block_index);
        
        int get_memory_size() const { return flash_memory_size; }
        int get_recently_access_sector(const int block_index) const { return flash_memory[block_index].recently_access_sector; }
        void set_recently_access_sector(const int block_index, bool init = false) const{ 
            if(init == true) { flash_memory[block_index].recently_access_sector = 0; }
            else { flash_memory[block_index].recently_access_sector++; }
        }
        bool get_is_using(const int block_index, const int sector_index) const { return flash_memory[block_index].block[sector_index].is_using; }
        void set_is_using(const int block_index, const int sector_index, bool option) const { 
            flash_memory[block_index].block[sector_index].is_using = option; 
        }

        BLOCK& get_one_block(const int block_index) const { return flash_memory[block_index]; }
        int get_block_wear_level(const int block_index) const { return flash_memory[block_index].wear_level; }
        FLASH_MEMORY(){};
        ~FLASH_MEMORY(){
            cout<<"FLASH_MEMORY::called ~FLASH_MEMORY()\n";
            delete[] flash_memory;
        };
        //////////for test
        void print_memory(const int from, const int to)const{
            for(int i = from; i < to; i++){
                cout<<"block_num : "<<i<<" index data using\n";
                for(int j = 0; j < BLOCK_SIZE; j++){
                    cout<<"sector_num : "<<j<<" "<<flash_memory[i].block[j].data<<"\t"<<flash_memory[i].block[j].is_using<<"\n";
                }
                cout<<"\n\n\n";
            }
        };
    private:
        BLOCK* flash_memory = nullptr;
        int flash_memory_size = 0;
        int input_size = 0;
};

class FTL{
public:
    bool init();
    bool FTL_write(const int index, const char data[]){
        int lbn = index / BLOCK_SIZE;
        int lsn = index % BLOCK_SIZE;
        if(index < 0 || lbn >= flash_memory.get_memory_size() || lsn >= BLOCK_SIZE) { 
            cout<<"FTL::FTL_write( "<<lbn<<", "<<lsn<<" ) :: try to access out of range\n";
            return false; 
        }
        if(block_mapping_table[lbn] == -1){
            if(data_block_Q.size() == 0){
                cout<<"FTL::FTL_write :: fail to assign data_block_Q, data_block_Q size : "<<data_block_Q.size()<<"\n";
                return false;
            }
            block_mapping_table[lbn] = data_block_Q.top().block_index;
            cout<<"FTL::FTL_write :: lbn( "<<lbn<<" ) assign pbn( "<<block_mapping_table[lbn]<<" )\n";
            data_block_Q.pop();
            if(block_mapping_table[lbn] == -1) {
                cout<<"FTL::FTL_write :: assign error, block_mapping_table\n";
                return false;
            }
        }
        cout<<"FTL::FTL_write :: lbn( "<<lbn<<" ) -> pbn( "<<block_mapping_table[lbn]<<" )\n";
        int pbn = block_mapping_table[lbn];
        if(flash_memory.flash_write(pbn, lsn, data) == true) {
            cout<<"FTL::FTL_write( "<<pbn<<", "<<lsn<<" ) :: "<<data<<"\n";
            return true;
        }
        if(sector_mapping_table[pbn].log_block == -1){
            if(log_block_Q.size() == 0){
                cout<<"FTL::FTL_write :: fail to assign log_block_Q, log_block_Q size : "<<log_block_Q.size()<<"\n";
                return false;
            }
            sector_mapping_table[pbn].log_block = log_block_Q.top().block_index;
            cout<<"FTL::FTL_write :: data_block( "<<lbn<<" ) : assign log_block( "<<sector_mapping_table[pbn].log_block<<" )\n";
            log_block_Q.pop();
            if(sector_mapping_table[pbn].log_block == -1) {
                cout<<"FTL::FTL_write :: assign error, sector_mapping_table\n";
                return false;
            }
        }
        cout<<"FTL::FTL_write :: lbn( "<<lbn<<" ) -> pbn( "<<block_mapping_table[lbn]<<" ) -> log_block( "<<sector_mapping_table[pbn].log_block<<" )\n";
        int access_sector_index = flash_memory.get_recently_access_sector(sector_mapping_table[pbn].log_block);
        if(flash_memory.flash_write(sector_mapping_table[pbn].log_block, access_sector_index, data) == true) {
            flash_memory.set_is_using(pbn, lsn, false);
            sector_mapping_table[pbn].sector_mapping[lsn] = access_sector_index;
            cout<<"FTL::FTL_write( "<<sector_mapping_table[pbn].log_block<<", "<<access_sector_index<<" ) :: "<<data<<"\n";
            if(access_sector_index == BLOCK_SIZE-1){
                cout<<"FTL::FTL_write :: call check_log_block_sequential\n";
                if(check_log_block_sequential(pbn) == true){
                    cout<<"FTL::FTL_write :: switch_operation\n";
                    switch_operation(lbn, pbn, sector_mapping_table[pbn].log_block);
                }
                else{
                    cout<<"FTL::FTL_write :: merge_operation\n";
                    merge_operation(lbn, pbn, sector_mapping_table[pbn].log_block);
                }
            }
            return true;

        }
        cout<<"FTL::FTL_write :: fail to update memory "<<lbn<<" "<<lsn<<"\n";
    
        return false;
    };
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
private:
    bool check_log_block_sequential(const int pbn){
        for(int i = 0; i < BLOCK_SIZE; i++){
            if(sector_mapping_table[pbn].sector_mapping[i] != i) { 
                cout<<"FTL::check_log_sequential :: pbn( "<<pbn<<" ) -> log_block( "<<sector_mapping_table[pbn].log_block<<" ) is not sequential\n";
                return false;
            }
        }
        cout<<"FTL::check_log_sequential :: pbn( "<<pbn<<" ) -> log_block( "<<sector_mapping_table[pbn].log_block<<" ) is sequential\n";
        return true;
    };
    bool merge_operation(const int lbn, const int pbn, const int log_pbn){
        if(data_block_Q.size() == 0) {
            cout<<"FTL::merge_operation :: fail to assign data_block_Q, data_block_Q size : "<<data_block_Q.size()<<"\n";
            return false;
        }
        int copy_block_index = data_block_Q.top().block_index;
        data_block_Q.pop();
        //////////////////////////////카피 블록을 할당할 때 로그큐와 데이터큐 중 비율 따져가며 할당하는 거 생각해봐
        //

        //
        //

        //
        //
        //
        //
        BLOCK& copy_block = flash_memory.get_one_block(copy_block_index);
        const BLOCK& data_block = flash_memory.get_one_block(pbn);
        const BLOCK& log_block = flash_memory.get_one_block(log_pbn);
        for(int i = 0; i < BLOCK_SIZE; i++){
            if(data_block.block[i].is_using == true) { strcpy_s(copy_block.block[i].data, data_block.block[i].data); }
            else { 
                int log_block_sector_index = sector_mapping_table[pbn].sector_mapping[i]; // log_block_sector_index = 로그 블록의 섹터 인덱스
                strcpy_s(copy_block.block[i].data, log_block.block[log_block_sector_index].data);
            } 
            copy_block.block[i].is_using = true;
        }
        cout<<"FTL::merge_operation :: copy complete (copy_block : "<<copy_block_index<<" )\n";
        if(flash_memory.flash_erase(pbn) == false){
            cout<<"FTL::merge_operation :: fail to erase block ( "<<pbn<<" )\n";
            return false;
        }
        if(flash_memory.flash_erase(log_pbn) == false){
            cout<<"FTL::merge_operation :: fail to erase block ( "<<log_pbn<<" )\n";
            return false;
        }
         //////////////////////////////////////push 할 때 로그블록 큐의 잔여량과 데이터 블록 큐의 잔여량을 비교하여 적은 쪽으로 푸쉬할 예정 ////
        //
        //
        //
        //

        //

        //
        //
        data_block_Q.push({data_block.wear_level, pbn});
        cout<<"FTL::merge_operation :: push to data_block_Q ( "<<pbn<<", "<<data_block.wear_level<<" )\n";
        log_block_Q.push({log_block.wear_level, log_pbn});
        cout<<"FTL::merge_operation :: push to log_block_Q ( "<<log_pbn<<", "<<log_block.wear_level<<" )\n";


        for(int i = 0; i < BLOCK_SIZE; i++) { sector_mapping_table[pbn].sector_mapping[i] = -1; }
        sector_mapping_table[pbn].log_block = -1;
        cout<<"FTL::merge_operation :: init the sector_mapping_table\n";
        block_mapping_table[lbn] = copy_block_index;
        cout<<"FTL::merge_operation :: assign new block"<<pbn<<" -> "<<copy_block_index<<"\n";
        return true;
    };
    bool switch_operation(const int lbn, const int pbn, const int log_pbn){
        block_mapping_table[lbn] = log_pbn; // switch
        cout<<"FTL::switch_operation :: switched block : "<<pbn<<" -> "<<log_pbn<<"\n";
        if(flash_memory.flash_erase(pbn) == false){
            cout<<"FTL::switch_operation :: fail to erase block "<<pbn<<" \n";
            return false;
        }
        for(int i = 0; i < BLOCK_SIZE; i++) { sector_mapping_table[pbn].sector_mapping[i] = -1; }
        sector_mapping_table[pbn].log_block = -1;
        cout<<"FTL::switch_operation :: init the sector_mapping_table\n";
        int data_block_wear_level = flash_memory.get_block_wear_level(pbn);
        //////////////////////////////////////push 할 때 로그블록 큐의 잔여량과 데이터 블록 큐의 잔여량을 비교하여 적은 쪽으로 푸쉬할 예정 ////
        //
        //
        //
        //

        //

        //
        //

        log_block_Q.push({data_block_wear_level, pbn});
        return true;
    };
    bool init_block_Q(const int flash_memory_size){
        data_block_ratio = (int)((1- LOG_BLOCK_RATIO) * flash_memory.get_memory_size());
        for(int i = 0; i < flash_memory.get_memory_size(); i++){
            if(i < data_block_ratio) { data_block_Q.push({0, i}); }
            else { log_block_Q.push({0, i}); }
        }
        if(log_block_Q.size() == 0 || data_block_Q.size() == 0){ return false; }
        return true;
    };
    FLASH_MEMORY flash_memory;
    priority_queue<BW_pair> log_block_Q;
    priority_queue<BW_pair> data_block_Q;
    int* block_mapping_table = nullptr;
    SMP* sector_mapping_table = nullptr;
    int data_block_ratio = 0;
};


int main(){
    FTL bast;
    bast.init();
    bast.test();
    return 0;
}


bool FLASH_MEMORY::init(){
    cout<<"FLASH_MEMORY::init :: enter falsh_memory size(mb) : ";
    //for_testcin>>input_size;
    input_size = 1;
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
    if(block_index > flash_memory_size || block_index < 0 || sector_index > BLOCK_SIZE || sector_index < 0){
        cout<<"FLASH_MEMORY::flash_write :: try to access out of range block_index : <<"<<block_index<<", sector_index : "<<sector_index<<"\n";
        return false;
    }
    if(flash_memory[block_index].block[sector_index].is_using == true
        || flash_memory[block_index].block[sector_index].data[0] != '\0') { 
        cout<<"FLASH_MEMORY::flash_write( "<<block_index<<", "<<sector_index<<" ) :: already using now\n";
        return false;
    }
    strcpy_s(flash_memory[block_index].block[sector_index].data, data);
    cout<<"FLASH_MEMORY::flash_write( "<<block_index<<", "<<sector_index<<" ) :: "<<flash_memory[block_index].block[sector_index].data<<"\n";
    flash_memory[block_index].block[sector_index].is_using = true;
    flash_memory[block_index].recently_access_sector++;
    return true;
};

bool FLASH_MEMORY::flash_read(const int block_index, const int sector_index){
    if(block_index > flash_memory_size || block_index < 0 || sector_index > BLOCK_SIZE || sector_index < 0){
        cout<<"FLASH_MEMORY::flash_erase :: try to access out of range block_index : <<"<<block_index<<", sector_index : "<<sector_index<<"\n";
        return false;
    }
    if(flash_memory[block_index].block[sector_index].is_using == true){
        cout<<"FLASH_MEMORY::flash_read( "<<block_index<<", "<<sector_index<<" ) :: "<<flash_memory[block_index].block[sector_index].data<<"\n";
        return true;
    }
    if(flash_memory[block_index].block[sector_index].data[0] != '\0'){
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
        strcpy_s(flash_memory[block_index].block[i].data, "");
        flash_memory[block_index].block[i].is_using = false;
    }
    flash_memory[block_index].recently_access_sector = 0;
    flash_memory[block_index].wear_level++;
    cout<<"FLASH_MEMORY::flash_erase( "<<block_index<<" ) :: complete\n";
    return true;
};

bool FTL::init(){
    int flash_memory_size = flash_memory.get_memory_size();
    int block_mapping_table_size = flash_memory_size;
    int sector_mapping_table_size = block_mapping_table_size * BLOCK_SIZE;
    block_mapping_table = new int[block_mapping_table_size];
    if(block_mapping_table == nullptr) {
        cout<<"FTL::init :: fail to create block_mapping_table\n";
        return false;
    }
    sector_mapping_table = new SMP[sector_mapping_table_size];
    if(sector_mapping_table == nullptr) {
        cout<<"FTL::init :: fail to create sector_mapping_table\n";
        return false;
    }
    for(int i = 0; i < flash_memory_size; i++) { block_mapping_table[i] = -1; }
    for(int i = 0; i < sector_mapping_table_size; i++){
        for(int j = 0; j < BLOCK_SIZE; j++) { sector_mapping_table[i].sector_mapping[j] = -1; }
    }
    if(init_block_Q(flash_memory_size) == false) {
        cout<<"FTL::init :: fail to init_block_Q\n";
        return false;
    }
    cout<<"FTL::init :: init data_block_Q, size : "<<data_block_Q.size()<<"\n";
    cout<<"FTL::init :: init log_block_Q, size : "<<log_block_Q.size()<<"\n";
    return true;
};

bool FTL::FTL_read(const int index){
    int lbn = index / BLOCK_SIZE;
    int lsn = index % BLOCK_SIZE;
    if(index < 0 || lbn >= flash_memory.get_memory_size() || lbn >= BLOCK_SIZE) {
        cout<<"FTL::FTL_read :: try to access out of range\n";
        return false;
    }
    int mapping_index_block = block_mapping_table[lbn];
    if(mapping_index_block == -1) {
        cout<<"FTL::FTL_read :: block_mapping assignment error\n";
        return false;
    }
    if(flash_memory.flash_read(mapping_index_block, lsn) == false){
        mapping_index_block = sector_mapping_table[lbn].log_block;
        if(mapping_index_block == -1) {
            cout<<"FTL::FTL_read :: sector_mapping assignment error "<<mapping_index_block<<", "<<lsn<<"\n";
            return false;
        }
        int mapping_index_sector = sector_mapping_table[lbn].sector_mapping[lsn];
        if(flash_memory.flash_read(mapping_index_block, mapping_index_sector) == false){
            cout<<"FTL::FTL_read :: fail to read "<<mapping_index_block<<", "<<mapping_index_sector<<"\n";
            return false;
        }
    }
    return true;
}


void FTL::test(){
    ifstream fin;
    fin.open("test.txt");
    if(fin.fail() == true){
        cout<<"fail to open file\n";
        return;
    }
    
    int option = 0;
    int index = 0;
    int index2 = 0;
    char ch[10] = {};
    while(fin.eof() == false){
        // cout<<"1. FTL_write\t2. FTL_read\t3. cls\t4. print_mapping_table\t5. print_memory(from ~ to)\n\n";
        // cin>>option;
        fin>>option;
        switch(option){
        case 1 :
            fin>>index>>ch;
            cout<<index<<" "<<ch<<"\n";
            FTL_write(index, ch);
            break;
        case 2 :
            fin>>index;
            cout<<index<<"\n";
            FTL_read(index);
            break;
        case 3 :
            system("cls");
            break;
        case 4 :
            //print_mapping();
            break;
        case 5 :
            fin>>index>>index2;
            cout<<index<<" "<<index2<<"\n";
            flash_memory.print_memory(index, index2);
            break;
        case 6 :
            break;
        }
    }
    fin.close();
};

void FTL::test2(){
    int option = 0;
    int index = 0;
    int index2 = 0;
    char ch[100] = {};
    while(1){
        cout<<"1. FTL_write\t2. FTL_read\t3. cls\t4. print_mapping_table\t5. print_memory(from ~ to)\n\n";
        cin>>option;
        switch(option){
        case 1 :
            cin>>index>>ch;
            cout<<index<<" "<<ch<<"\n";
            FTL_write(index, ch);
            break;
        case 2 :
            cin>>index;
            cout<<index<<"\n";
            FTL_read(index);
            break;
        case 3 :
            system("cls");
            break;
        case 4 :
            // print_mapping();
            break;
        case 5 :
            cin>>index>>index2;
            cout<<index<<" "<<index2<<"\n";
            flash_memory.print_memory(index, index2);
            break;
        case 6 :
            break;
        }
    }
};