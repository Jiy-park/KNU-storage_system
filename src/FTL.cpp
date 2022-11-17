#include"ftl.h"
#include<ctime>

bool FTL::init(){
    int flash_memory_size = flash_memory.get_memory_size();
    int BMT_size = flash_memory_size;
    int log_BMT_size = BMT_size * BLOCK_SIZE;
    BMT = new BMS[BMT_size];
    if(BMT == nullptr) {
        cout<<"FTL::init :: fail to create BMT\n";
        return false;
    }
    log_BMT = new LBMS[log_BMT_size];
    if(log_BMT == nullptr) {
        cout<<"FTL::init :: fail to create log_BMT\n";
        return false;
    }
    // for(int i = 0; i < flash_memory_size; i++) { BMT[i].pbn = -1; }
    for(int i = 0; i < log_BMT_size; i++){
        for(int j = 0; j < BLOCK_SIZE; j++) { log_BMT[i].sector_mapping[j] = -1; }
    }
    if(init_block_Q(flash_memory_size) == false) {
        cout<<"FTL::init :: fail to init_block_Q\n";
        return false;
    }
    cout<<"FTL::init :: init data_block_Q, size : "<<data_block_Q.size()<<"\n";
    cout<<"FTL::init :: init log_block_Q, size : "<<log_block_Q.size()<<"\n";
    return true;
};

bool FTL::FTL_write(const int index, const char data[]){
    cout<<"input_count : "<<test.input_count<<"\n";
    test.input_count++;
    int lbn = index / BLOCK_SIZE;
    int lsn = index % BLOCK_SIZE;
    if(index < 0 || lbn >= flash_memory.get_memory_size() || lsn >= BLOCK_SIZE) { 
        cout<<"FTL::FTL_write( "<<lbn<<", "<<lsn<<" ) :: try to access out of range\n";
        return false; 
    }
    if(BMT[lbn].pbn == -1){
        if(data_block_Q.size() == 0){
            cout<<"FTL::FTL_write :: fail to assign data_block_Q, data_block_Q size : "<<data_block_Q.size()<<"\n";
            return false;
        }
        BMT[lbn].pbn = data_block_Q.top().block_index;
        cout<<"FTL::FTL_write :: lbn( "<<lbn<<" ) assign pbn( "<<BMT[lbn].pbn<<" )\n";
        data_block_Q.pop();
        if(BMT[lbn].pbn == -1) {
            cout<<"FTL::FTL_write :: assign error, BMT\n";
            return false;
        }
    }
    cout<<"FTL::FTL_write :: lbn( "<<lbn<<" ) -> pbn( "<<BMT[lbn].pbn<<" )\n";
    int pbn = BMT[lbn].pbn;
    test.start = clock();
    if(flash_memory.flash_write(pbn, lsn, data) == true) {
        cout<<"FTL::FTL_write( "<<pbn<<", "<<lsn<<" ) :: "<<data<<"\n";
        test.end = clock();
        test.write_time.push_back({test.input_count, (test.end - test.start)});
        test.read_count++;
        test.write_count++;
        return true;
    }

    //log_block에 기입
    if(lsn == 0 && log_BMT[pbn].log_block != -1) {
        test.start = clock();
        merge_operation(lbn, pbn, log_BMT[pbn].log_block); 
        test.end = clock();
        test.merge_operation_time.push_back({test.input_count, (test.end - test.start)});
        test.merge_count++;  
    }
    pbn = BMT[lbn].pbn;
    if(log_BMT[pbn].log_block == -1){
        if(log_block_Q.size() == 0){
            cout<<"FTL::FTL_write :: fail to assign log_block_Q, log_block_Q size : "<<log_block_Q.size()<<"\n";
            return false;
        }
        log_BMT[pbn].log_block = log_block_Q.top().block_index;
        cout<<"FTL::FTL_write :: data_block( "<<lbn<<" ) : assign log_block( "<<log_BMT[pbn].log_block<<" )\n";
        log_block_Q.pop();
        if(log_BMT[pbn].log_block == -1) {
            cout<<"FTL::FTL_write :: assign error, log_BMT\n";
            return false;
        }
    }
    cout<<"FTL::FTL_write :: lbn( "<<lbn<<" ) -> pbn( "<<BMT[lbn].pbn<<" ) -> log_block( "<<log_BMT[pbn].log_block<<" )\n";
    int access_sector_index = log_BMT[pbn].recently_access_sector;
    //for_test
    test.start = clock();
    //
    if(flash_memory.flash_write(log_BMT[pbn].log_block, access_sector_index, data) == true) {
        //for_test
        test.end = clock();
        test.write_time.push_back({test.input_count, (test.end - test.start)});
        //
        log_BMT[pbn].recently_access_sector++;
        flash_memory.set_is_using(pbn, lsn, false);
        log_BMT[pbn].sector_mapping[lsn] = access_sector_index;
        cout<<"FTL::FTL_write( "<<log_BMT[pbn].log_block<<", "<<access_sector_index<<" ) :: "<<data<<"\n";
        if(access_sector_index == BLOCK_SIZE-1){
            cout<<"FTL::FTL_write :: check_log_block_sequential\n";
            for(int i = 0; i < BLOCK_SIZE; i++){
                if(log_BMT[pbn].sector_mapping[i] != i) { 
                    cout<<"FTL::FTL_write :: merge_operation ("<<lbn<<", "<< pbn<<", "<<log_BMT[pbn].log_block<<")\n";
                    test.start = clock();
                    merge_operation(lbn, pbn, log_BMT[pbn].log_block);
                    test.end = clock();
                    test.merge_operation_time.push_back({test.input_count, (test.end - test.start)});
                    cout<<"\n\n\n\n";
                    test.merge_count++;
                    test.read_count += 2;
                    test.write_count++;
                    return true;
                }
            }
            cout<<"FTL::FTL_write :: switch_operation\n";
            test.start = clock();
            switch_operation(lbn, pbn, log_BMT[pbn].log_block);
            test.end = clock();
            test.switch_operation_time.push_back({test.input_count, (test.end - test.start)});
            cout<<"\n\n\n\n";
            test.switch_count++;
        }
        test.read_count += 2;
        test.write_count++;
        return true;
    }
    cout<<"FTL::FTL_write :: fail to update memory "<<lbn<<" "<<lsn<<"\n";
    test.write_fail_count++;
    return false;
};



bool FTL::FTL_read(const int index){
    int lbn = index / BLOCK_SIZE;
    int lsn = index % BLOCK_SIZE;
    if(index < 0 || lbn >= flash_memory.get_memory_size() || lbn >= BLOCK_SIZE) {
        cout<<"FTL::FTL_read :: try to access out of range\n";
        return false;
    }
    int pbn = BMT[lbn].pbn;
    if(pbn == -1) {
        cout<<"FTL::FTL_read :: block_mapping assignment error\n";
        return false;
    }
    test.start = clock();
    if(flash_memory.flash_read(pbn, lsn) == false){
        pbn = log_BMT[lbn].log_block;
        if(pbn == -1) {
            cout<<"FTL::FTL_read :: sector_mapping assignment error "<<pbn<<", "<<lsn<<"\n";
            return false;
        }
        int mapping_index_sector = log_BMT[lbn].sector_mapping[lsn];
        if(flash_memory.flash_read(pbn, mapping_index_sector) == false){
            cout<<"FTL::FTL_read :: fail to read "<<pbn<<", "<<mapping_index_sector<<"\n";
            return false;
        }
    }
    test.end = clock();
    test.read_time.push_back({test.input_count, (test.end - test.start)});
    test.read_count++;
    return true;
}

bool FTL::merge_operation(const int lbn, const int pbn, const int log_pbn){   
    if(data_block_Q.size() == 0) {
        cout<<"FTL::merge_operation :: fail to assign data_block_Q, data_block_Q size : "<<data_block_Q.size()<<"\n";
        return false;
    }
    int copy_block_index = data_block_Q.top().block_index;
    data_block_Q.pop();
    BLOCK& copy_block = flash_memory.get_one_block(copy_block_index);
    const BLOCK& data_block = flash_memory.get_one_block(pbn);
    const BLOCK& log_block = flash_memory.get_one_block(log_pbn);
    for(int i = 0; i < BLOCK_SIZE; i++){
        if(data_block.sector[i].is_using == true) { strcpy_s(copy_block.sector[i].data, data_block.sector[i].data);}
        else { 
            int log_block_sector_index = log_BMT[pbn].sector_mapping[i]; // log_block_sector_index = 로그 블록의 섹터 인덱스
            strcpy_s(copy_block.sector[i].data, log_block.sector[log_block_sector_index].data);
        } 
        copy_block.sector[i].is_using = true;
    }
    cout<<"FTL::merge_operation :: copy complete (copy_block : "<<copy_block_index<<" )\n";
    test.start = clock();
    if(flash_memory.flash_erase(pbn) == false){
        cout<<"FTL::merge_operation :: fail to erase block ( "<<pbn<<" )\n";
        return false;
    }
    test.end = clock();
    test.erase_time.push_back({test.input_count, (test.end - test.start)});
    log_BMT[pbn].recently_access_sector = 0;
    test.start = clock();
    if(flash_memory.flash_erase(log_pbn) == false){
        cout<<"FTL::merge_operation :: fail to erase block ( "<<log_pbn<<" )\n";
        return false;
    }
    test.end = clock();
    test.erase_time.push_back({test.input_count, (test.end - test.start)});
    log_BMT[log_pbn].recently_access_sector = 0;
    if(cmp_Q_size(data_block.wear_level, pbn) == false){
        cout<<"FTL::merge_operation :: fail to cmp_Q_size( "<<data_block.wear_level<<", "<<pbn<<" )\n";
        return false;
    }
    cout<<"FTL::merge_operation :: push to block_Q ( "<<data_block.wear_level<<", "<<pbn<<" )\n";
    if(cmp_Q_size(log_block.wear_level, log_pbn) == false){
        cout<<"FTL::merge_operation :: fail to cmp_Q_size( "<<log_block.wear_level<<", "<<log_pbn<<" )\n";
        return false;
    }
    cout<<"FTL::merge_operation :: push to block_Q ( "<<log_block.wear_level<<", "<<log_pbn<<" )\n";
    for(int i = 0; i < BLOCK_SIZE; i++) { log_BMT[pbn].sector_mapping[i] = -1; }
    log_BMT[pbn].log_block = -1;
    cout<<"FTL::merge_operation :: init the log_BMT\n";
    BMT[lbn].pbn = copy_block_index;
    cout<<"FTL::merge_operation :: assign new block"<<pbn<<" -> "<<copy_block_index<<"\n";
    test.erase_count += 2;
    return true;
};

bool FTL::switch_operation(const int lbn, const int pbn, const int log_pbn){
    cout<<"\n\n\n\n";
    BMT[lbn].pbn = log_pbn; // switch
    cout<<"FTL::switch_operation :: switched block : "<<pbn<<" -> "<<log_pbn<<"\n";
    test.start = clock();
    if(flash_memory.flash_erase(pbn) == false){
        cout<<"FTL::switch_operation :: fail to erase block "<<pbn<<" \n";
        return false;
    }
    test.end = clock();
    test.erase_time.push_back({test.input_count, (test.end - test.start)});
    log_BMT[pbn].recently_access_sector = 0;
    for(int i = 0; i < BLOCK_SIZE; i++) { log_BMT[pbn].sector_mapping[i] = -1; }
    log_BMT[pbn].log_block = -1;
    cout<<"FTL::switch_operation :: init the log_BMT\n";
    int pbn_wear_level = flash_memory.get_block_wear_level(pbn);
    if(cmp_Q_size(pbn_wear_level, pbn) == false){
        cout<<"FTL::switch_operation :: fail to cmp_Q_size( "<<pbn_wear_level<<", "<<pbn<<" )\n";
        return false;
    }
    test.erase_count++;
    return true;
};

bool FTL::init_block_Q(const int flash_memory_size){
    data_block_ratio = (int)((1- LOG_BLOCK_RATIO) * flash_memory_size);
    for(int i = 0; i < flash_memory.get_memory_size(); i++){
        if(i < data_block_ratio) { data_block_Q.push({0, i}); }
        else { log_block_Q.push({0, i}); }
    }
    if(log_block_Q.size() == 0 || data_block_Q.size() == 0){ return false; }
    return true;
};

bool FTL::cmp_Q_size(const int wear_level, const int index){
    if(index >= flash_memory.get_memory_size()) {
        cout<<"FTL::cmp_Q_size :: index( "<<index<<" ) is out of range\n";
        return false;
    }
    float DQ_remaining_amount = (float)data_block_Q.size() / data_block_ratio; // DQ 잔여량
    float LQ_remaining_amount = (float)log_block_Q.size() / (flash_memory.get_memory_size() - data_block_ratio); // LQ 잔여량
    if(LQ_remaining_amount >= DQ_remaining_amount) {
        data_block_Q.push({wear_level, index}); 
        cout<<"FTL::cmp_Q_size :: index( "<<index<<" ) is pushed to data_block_Q, wear_level : "<<wear_level<<" size : "<<data_block_Q.size()<<"\n";
    } 
    else {
        log_block_Q.push({wear_level, index});
        cout<<"FTL::cmp_Q_size :: index( "<<index<<" ) is pushed to log_block_Q, wear_level : "<<wear_level<<" size : "<<log_block_Q.size()<<"\n";
    }
    cout<<"FTL::cmp_Q_size :: data_block_Q  remaining_amount : "<<DQ_remaining_amount<<", size : "<<data_block_Q.size()<<"\n";
    cout<<"FTL::cmp_Q_size :: log_block_Q  remaining_amount : "<<LQ_remaining_amount<<", size : "<<log_block_Q.size()<<"\n";
    return true;
}

void FTL::test1(){
    time_t start, end;
    ifstream fin;
    fin.open("../../test.txt");
    if(fin.fail() == true){
        cout<<"fail to open file\n";
        return;
    }
    
    int option = 0;
    int index = 0;
    int index2 = 0;
    char ch[100] = {};
    start = clock();
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
    end = clock();
    cout<<"\n\nrun time : "<<(end - start)<<" ms \n";
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


void FTL::test3(){
    time_t start, end;
    ifstream fin;
    fin.open("../../linux.txt");
    if(fin.fail() == true){
        cout<<"fail to open file\n";
        return;
    }
    
    char command = ' ';
    int index = 0;
    char data[512] = "init";
    start = clock();
    while(fin.eof() == false){
        fin>>command>>index;
        cout<<index<<" "<<data<<"\n";
        FTL_write(index, data);
    }
    end = clock();
    cout<<"\n\nrun time : "<<end - start<<" ms \n";
    fin.close();
};

void FTL::FOR_TEST::fout_write_time(){
    fstream f;
    f.open("../../write_time.txt");
    if(f.fail() == true){
        cout<<"fail to open file ( write_time.txt )\n";
        return;
    }
    cout<<"start write file ( write_time.txt )\n";
    for(int i = 0; i < write_time.size(); i++) { f<<write_time[i].input_count<<" "<<write_time[i].operation_time<<"\n"; }
    cout<<"end write file ( write_time.txt )\n";
    f.close();
}

void FTL::FOR_TEST::fout_read_time(){
    fstream f;
    f.open("../../read_time.txt");
    if(f.fail() == true){
        cout<<"fail to open file ( read_time.txt )\n";
        return;
    }
    cout<<"start read file ( read_time.txt )\n";
    for(int i = 0; i < read_time.size(); i++) { f<<read_time[i].input_count<<" "<<read_time[i].operation_time<<"\n"; }
    cout<<"end write file ( read_time.txt )\n";
    f.close();
}

void FTL::FOR_TEST::fout_erase_time(){
    fstream f;
    f.open("../../erase_time.txt");
    if(f.fail() == true){
        cout<<"fail to open file ( erase_time.txt )\n";
        return;
    }
    cout<<"start erase file ( erase_time.txt )\n";
    for(int i = 0; i < erase_time.size(); i++) { f<<erase_time[i].input_count<<" "<<erase_time[i].operation_time<<"\n"; }
    cout<<"end write file ( erase_time.txt )\n";
    f.close();
}

void FTL::FOR_TEST::fout_merge_operation_time(){
    fstream f;
    f.open("../../merge_operation_time.txt");
    if(f.fail() == true){
        cout<<"fail to open file ( merge_operation_time.txt )\n";
        return;
    }
    cout<<"start merge_operation file ( merge_operation_time.txt )\n";
    for(int i = 0; i < merge_operation_time.size(); i++) { f<<merge_operation_time[i].input_count<<" "<<merge_operation_time[i].operation_time<<"\n"; }
    cout<<"end write file ( merge_operation_time.txt )\n";
    f.close();
}

void FTL::FOR_TEST::fout_switch_operation_time(){
    fstream f;
    f.open("../../switch_operation_time.txt");
    if(f.fail() == true){
        cout<<"fail to open file ( switch_operation_time.txt )\n";
        return;
    }
    cout<<"start switch_operation file ( switch_operation_time.txt )\n";
    for(int i = 0; i < switch_operation_time.size(); i++) { f<<switch_operation_time[i].input_count<<" "<<switch_operation_time[i].operation_time<<"\n"; }
    cout<<"end write file ( switch_operation_time.txt )\n";
    f.close();
}