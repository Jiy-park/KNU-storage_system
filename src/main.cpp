#include<iostream>
#include<queue>
#include<fstream>
using namespace std;


//MEGABYTE_SIZE = sizeof(BLOCK) * BLOCK_SIZE * num of block in memory 

#define MEGABYTE_SIZE 1024*1024
#define SECTOR_SIZE 512
#define BLOCK_SIZE 4
#define LOG_BLOCK_RATIO 0.1

struct SECTOR{
    char data[SECTOR_SIZE] = "";
    bool is_using = false;
    int replace_sector = -1;
};

struct BLOCK{
    SECTOR block[BLOCK_SIZE] = {};
    int recently_access_sector = 0;
    int wear_level = 0;
    
};

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
        bool flash_write(const int block_index, const int sector_index, const char data[], const int replace = -1);
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

        int get_replace_sector(const int block_index, const int sector_index) const { return flash_memory[block_index].block[sector_index].replace_sector; }
        void set_replace_sector(const int block_index, const int sector_index, const int replace_index) const {
            flash_memory[block_index].block[sector_index].replace_sector = replace_index;
        }
        BLOCK& get_one_block(const int block_index) const { return flash_memory[block_index]; }
        FLASH_MEMORY(){};
        ~FLASH_MEMORY(){
            cout<<"FLASH_MEMORY::called ~FLASH_MEMORY()\n";
            delete[] flash_memory;
        };
        //////////for test



        
        void print_memory(const int from, const int to)const{
            for(int i = from; i < to; i++){
                cout<<"block_num : "<<i<<"index data using replace\n";
                for(int j = 0; j < BLOCK_SIZE; j++){
                    cout<<"sector_num : "<<j<<" "<<flash_memory[i].block[j].data<<"\t"<<flash_memory[i].block[j].is_using<<"\t"<<flash_memory[i].block[j].replace_sector<<"\n";
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
    bool FTL_write(const int index, const char data[]);
    bool FTL_read(const int index);

    FTL(){ flash_memory.init(); };
    ~FTL(){
        cout<<"FTL::called ~FTL()\n";
        delete[] mapping_table;
    };
    /////////for test
    void test();
    void test2();
    void print_mapping(){
        for(int i = 0; i < mapping_table_size; i++){
            if(i%5 == 0) { cout<<"\n\n"; }
            cout<<"index : "<<i<<" element : "<<mapping_table[i]<<"\n";
        }
    };
private:
    bool merge_operation(const int block_index, bool switch_mode = false);
    bool init_log_block_Q(const int flash_memory_size);
    FLASH_MEMORY flash_memory;
    priority_queue<BW_pair> log_block_Q;
    int* mapping_table = nullptr;
    int mapping_table_size = 0;
    int LB_ratio = 0;
};


int main(){
    FTL bast;
    bast.init();
    bast.test();
    return 0;
}






bool FLASH_MEMORY::init(){
    cout<<"FLASH_MEMORY::init :: enter falsh_memory size(mb) : ";
    cin>>input_size;
    flash_memory_size = (int)(input_size * MEGABYTE_SIZE / sizeof(BLOCK));
    flash_memory = new BLOCK[flash_memory_size];
    if(flash_memory == nullptr) { 
        cout<<"FLASH_MEMORY::init :: fail to create flash_memory\n";
        return false;
    }
    cout<<"FLASH_MEMORY::init :: created flash_memory, size : "<<flash_memory_size<<"\n";
    return true;
};


bool FLASH_MEMORY::flash_write(const int block_index, const int sector_index, const char data[], const int replace){
    if(flash_memory[block_index].block[sector_index].is_using == true
        || flash_memory[block_index].block[sector_index].data[0] != '\0') { 
        cout<<"FLASH_MEMORY::flash_write("<<block_index<<", "<<sector_index<<") :: already using now\n";
        return false;
    }
    strcpy_s(flash_memory[block_index].block[sector_index].data, data);
    cout<<"FLASH_MEMORY::flash_write("<<block_index<<", "<<sector_index<<") :: "<<flash_memory[block_index].block[sector_index].data<<"\n";
    flash_memory[block_index].block[sector_index].is_using = true;
    flash_memory[block_index].block[sector_index].replace_sector = replace;
    return true;
};

bool FLASH_MEMORY::flash_read(const int block_index, const int sector_index){
    if(flash_memory[block_index].block[sector_index].is_using == true){
        cout<<"FLASH_MEMORY::flash_read("<<block_index<<", "<<sector_index<<") :: "<<flash_memory[block_index].block[sector_index].data<<"\n";
        return true;
    }
    if(flash_memory[block_index].block[sector_index].data[0] != '\0'){
        cout<<"FLASH_MEMORY::flash_read("<<block_index<<", "<<sector_index<<") :: data was updated\n";
        return false;
    }
    cout<<"FLASH_MEMORY::flash_read("<<block_index<<", "<<sector_index<<") :: no data\n";
    return true;
};

bool FLASH_MEMORY::flash_erase(const int block_index){
    if(block_index < 0 || block_index > flash_memory_size) {
        cout<<"FLASH_MEMORY::flash_erase("<<block_index<<") :: try to access out of range\n";
        return false;
    }
    for(int i = 0; i < BLOCK_SIZE; i++){
        strcpy_s(flash_memory[block_index].block[i].data, "");
        flash_memory[block_index].block[i].is_using = false;
        flash_memory[block_index].block[i].replace_sector = -1;
    }
    flash_memory[block_index].recently_access_sector = 0;
    flash_memory[block_index].wear_level++;
    cout<<"FLASH_MEMORY::flash_erase("<<block_index<<") :: complete\n";
    return true;
};


bool FTL::init(){
    int flash_memory_size = flash_memory.get_memory_size();
    LB_ratio = (int)((1- LOG_BLOCK_RATIO) * flash_memory_size);
    mapping_table_size = LB_ratio;
    mapping_table = new int[mapping_table_size];
    if(mapping_table == nullptr) {
        cout<<"FTL::init :: fail to create mapping table\n";
        return false;
    }
    for(int i = 0; i < mapping_table_size; i++) { mapping_table[i] = -1; }
    cout<<"FTL::init :: created mapping table, size : "<<mapping_table_size<<"\n";
    if(init_log_block_Q(flash_memory_size) == false) {
        cout<<"FTL::init :: fail to init_log_block_Q\n";
        return false;
    }
    cout<<"FTL::init :: created log_block_Q, size : "<<log_block_Q.size()<<"\n";
    return true;
};

bool FTL::init_log_block_Q(const int flash_memory_size) {
    for(int i = LB_ratio; i < flash_memory_size; i++) { log_block_Q.push({0, i}); }
    if(flash_memory_size != 0 && log_block_Q.size() == 0) { return false; }
    return true;
};

bool FTL::FTL_write(const int index, const char data[]){
    int block_index = index / BLOCK_SIZE;
    int sector_index = index % BLOCK_SIZE;
    if(index < 0 || block_index > flash_memory.get_memory_size() || sector_index > BLOCK_SIZE) { 
        cout<<"FTL::FTL_write("<<block_index<<", "<<sector_index<<") :: try to access out of range\n";
        return false; 
    }
    if(flash_memory.flash_write(block_index, sector_index, data) == true) {
        cout<<"FTL::FTL_write("<<block_index<<", "<<sector_index<<") :: "<<data<<"\n";
        return true;
    }
    if(mapping_table[block_index] == -1){
        if(log_block_Q.size() == 0) { 
            cout<<"FTL::FTL_write :: fail to assign log_block, log_block size : "<<log_block_Q.size()<<"\n";
            return false;
        }
        mapping_table[block_index] = log_block_Q.top().block_index;
        log_block_Q.pop();
    }
    int log_block_index = mapping_table[block_index];
    int log_sector_index = flash_memory.get_recently_access_sector(log_block_index);
    if(log_sector_index == BLOCK_SIZE){
        if(merge_operation(block_index) == false){
            cout<<"FTL::FTL_write :: fail to merge_operation block_index : "<<block_index<<"\n";
            return false;
        }
        if(flash_memory.flash_write(block_index, sector_index, data) == false){
            cout<<"FTL::FTL_write :: fail to update memory after merge)operation\n";
            return false;
        }
        cout<<"FTL::FTL_write("<<block_index<<", "<<sector_index<<") :: "<<data<<"\n";
        return true;
    }
    if(flash_memory.flash_write(log_block_index, log_sector_index, data) == true){
        flash_memory.set_recently_access_sector(log_block_index);
        flash_memory.set_is_using(block_index, sector_index, false);
        flash_memory.set_replace_sector(block_index, sector_index, log_sector_index);
        cout<<"FTL::FTL_write("<<block_index<<", "<<sector_index<<") :: "<<data<<"\n";
        return true;
    }
    cout<<"FTL::FTL_write :: fail to update memory "<<log_block_index<<" "<<log_sector_index<<"\n";
    return false;
};

bool FTL::FTL_read(const int index){ 
    int block_index = index / BLOCK_SIZE;
    int sector_index = index % BLOCK_SIZE;
    if(index < 0 || block_index > flash_memory.get_memory_size() || sector_index > BLOCK_SIZE) {
        cout<<"FTL::FTL_read :: try to access out of range\n";
        return false;
    }
    if(flash_memory.flash_read(block_index, sector_index) == false){
        int log_block_index = mapping_table[block_index];
        int log_sector_index = flash_memory.get_replace_sector(block_index, sector_index);
        if(log_block_index == -1) {
            cout<<"FTL::FTL_read :: assignment error\n";
            return false;
        }
        if(flash_memory.flash_read(log_block_index, log_sector_index) == false){
            cout<<"이게 왜 안 돼????\n";
            return false;
        }
        return true;
    }
    return true;
};

bool FTL::merge_operation(const int block_index, bool switch_mode){
    if(block_index > flash_memory.get_memory_size() || block_index < 0){
        cout<<"FTL::merge_operation :: try to access out of range\n";
        return false;
    }
    if(log_block_Q.size() == 0) { 
        cout<<"FTL::merge_operation :: fail to assign log_block, log_block size : "<<log_block_Q.size()<<"\n";
        return false;
    }
    int replace_block_index = log_block_Q.top().block_index;
    int org_log_block_index = mapping_table[block_index];
    const BLOCK org_block = flash_memory.get_one_block(block_index);
    const BLOCK org_log_block = flash_memory.get_one_block(org_log_block_index);
    BLOCK copy_block = flash_memory.get_one_block(replace_block_index);

    for(int i = 0; i < BLOCK_SIZE; i++){
        if(org_block.block[i].is_using == true) { strcpy_s(copy_block.block[i].data, org_block.block[i].data); }
        else { 
            int log_replace_index = org_block.block[i].replace_sector;    
            strcpy_s(copy_block.block[i].data, org_log_block.block[log_replace_index].data); 
        } 
        copy_block.block[i].is_using = true;
        copy_block.block[i].replace_sector = -1;
    }
    cout<<"FTL::merge_operation :: copy complete\n";
    if(flash_memory.flash_erase(block_index) == false){
        cout<<"FTL::merge_operation :: fail to erase block "<<block_index<<" \n";
        return false;
    }
    if(flash_memory.flash_erase(org_log_block_index) == false){
        cout<<"FTL::merge_operation :: fail to erase block "<<org_log_block_index<<" \n";
        return false;
    }
    int log_block_wear_level = org_log_block.wear_level;
    log_block_Q.push({org_log_block_index, log_block_wear_level});
    cout<<"FTL::merge_operation :: push to log_block_Q ("<<org_log_block_index<<", "<<log_block_wear_level<<")\n";
    mapping_table[block_index] = replace_block_index;
    cout<<"FTL::merge_operation :: change mapping_table "<<org_log_block_index<<" -> "<<replace_block_index<<"\n";
    return true;
};

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
            print_mapping();
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
    char ch[10] = {};
    while(cin.eof() == false){
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
            print_mapping();
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