#include"ftl.h"

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
    //for test
    cout<<"input_count : "<<input_count<<"\n";
    input_count++;
    //
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
    if(flash_memory.flash_write(pbn, lsn, data) == true) {
        cout<<"FTL::FTL_write( "<<pbn<<", "<<lsn<<" ) :: "<<data<<"\n";
        //for_test
        read_count++;
        write_count++;
        //
        return true;
    }

    //log_block에 기입
    if(lsn == 0 && log_BMT[pbn].log_block != -1) { 
        merge_operation(lbn, pbn, log_BMT[pbn].log_block); 
        //for_test
        merge_count++;
        //   
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
    if(flash_memory.flash_write(log_BMT[pbn].log_block, access_sector_index, data) == true) {
        log_BMT[pbn].recently_access_sector++;
        flash_memory.set_is_using(pbn, lsn, false);
        log_BMT[pbn].sector_mapping[lsn] = access_sector_index;
        cout<<"FTL::FTL_write( "<<log_BMT[pbn].log_block<<", "<<access_sector_index<<" ) :: "<<data<<"\n";
        if(access_sector_index == BLOCK_SIZE-1){
            cout<<"FTL::FTL_write :: check_log_block_sequential\n";
            for(int i = 0; i < BLOCK_SIZE; i++){
                if(log_BMT[pbn].sector_mapping[i] != i) { 
                    cout<<"FTL::FTL_write :: merge_operation ("<<lbn<<", "<< pbn<<", "<<log_BMT[pbn].log_block<<")\n";
                    merge_operation(lbn, pbn, log_BMT[pbn].log_block);
                    cout<<"\n\n\n\n";
                    //for_test
                    merge_count++;
                    read_count += 2;
                    write_count++;
                    //
                    return true;
                }
            }
            cout<<"FTL::FTL_write :: switch_operation\n";
            switch_operation(lbn, pbn, log_BMT[pbn].log_block);
            cout<<"\n\n\n\n";
            //for_test
            switch_count++;
            //
        }
        //for_test
        read_count += 2;
        write_count++;
        //
        return true;
    }
    cout<<"FTL::FTL_write :: fail to update memory "<<lbn<<" "<<lsn<<"\n";
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
    //for_test
    read_count++;
    //
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
    if(flash_memory.flash_erase(pbn) == false){
        cout<<"FTL::merge_operation :: fail to erase block ( "<<pbn<<" )\n";
        return false;
    }
    log_BMT[pbn].recently_access_sector = 0;
    if(flash_memory.flash_erase(log_pbn) == false){
        cout<<"FTL::merge_operation :: fail to erase block ( "<<log_pbn<<" )\n";
        return false;
    }
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
    //for_test
    erase_count += 2;
    //
    return true;
};

bool FTL::switch_operation(const int lbn, const int pbn, const int log_pbn){
    cout<<"\n\n\n\n";
    BMT[lbn].pbn = log_pbn; // switch
    cout<<"FTL::switch_operation :: switched block : "<<pbn<<" -> "<<log_pbn<<"\n";
    if(flash_memory.flash_erase(pbn) == false){
        cout<<"FTL::switch_operation :: fail to erase block "<<pbn<<" \n";
        return false;
    }
    log_BMT[pbn].recently_access_sector = 0;
    for(int i = 0; i < BLOCK_SIZE; i++) { log_BMT[pbn].sector_mapping[i] = -1; }
    log_BMT[pbn].log_block = -1;
    cout<<"FTL::switch_operation :: init the log_BMT\n";
    int pbn_wear_level = flash_memory.get_block_wear_level(pbn);
    if(cmp_Q_size(pbn_wear_level, pbn) == false){
        cout<<"FTL::switch_operation :: fail to cmp_Q_size( "<<pbn_wear_level<<", "<<pbn<<" )\n";
        return false;
    }
    //for_test
    erase_count++;
    //
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

void FTL::test(){
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