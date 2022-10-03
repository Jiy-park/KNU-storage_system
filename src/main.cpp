#include <iostream>
#include <queue>
#include <stack>
#include <vector>
using namespace std;

#define SS 4
#define BS 32
#define log_block_rate 0.4

int Mapping_Table_sector[200];
int Mapping_Table_log[200];
int ST = (1 - log_block_rate) * BS;

struct Sector
{
    int Is_used;
    int pre_position = -1;
    // 0 = 사용 안함,  1 = new 데이터, 2 = old 데이터
    string Data;
};
struct Block
{
    bool operator<(const Block B) const
    {
        return this->wear > B.wear;
    }
    int wear = 0;
    Sector sector[SS];
    int log_block = -1;
    int BI =ST;
    
};
priority_queue<Block> use_able_log_block;
queue<Block> used_block;
Block arr[BS];

void Make_Log_Block()
{
    
    for(int i = ST; i<BS; i++)
    {
        arr[i].BI = i;
        use_able_log_block.push(arr[i]);
    }
}
void FTL_erase()
{
    
}

void switch_operation(int n)
{
    for(int i=0; i<ST; i++)
    {
        bool check = false;
        for(int k=0; k<SS; k++)
        {
            if(arr[i].sector[k].Is_used == false) continue;
            check = true;
            break;
        }
        
        // 여기에 프리 블록이 없을 경우 할당하는 프로그램 생성
        
        if(check == false)
        {
            for(int k=0; k<SS; k++)
            {
                arr[i].sector[k].Data = arr[n].sector[k].Data;
                arr[i].sector[k].Is_used = arr[n].sector[k].Is_used;
                arr[i].sector[k].pre_position = arr[n].sector[k].pre_position+SS;
                
                arr[n].sector[k].Data = "";
                arr[n].sector[k].Is_used = false;
                arr[n].sector[k].pre_position = -1;
            }
        }
    }
    
    use_able_log_block.push(used_block.front());
}

void merge_operation(int n)
{
    cout << n << endl;
    queue<pair<string,pair<int,int>>> Q;
    for(int i=0; i<ST; i++)
    {
        
        if(arr[i].log_block != -1)
        {
            for(int k=0; k<SS; k++)
            {
                if(arr[i].sector[k].Is_used == 1)
                {
                    //cout << arr[i].sector[k].Is_used << " " << arr[i].sector[k].Data << endl;
                    Q.push({arr[i].sector[k].Data,{arr[i].sector[k].pre_position, arr[i].sector[k].Is_used}});
                }
                
                arr[i].sector[k].Data = "";
                arr[i].sector[k].Is_used = false;
                arr[i].sector[k].pre_position = -1;
            }
        }
    }
    for(int k=0; k<SS; k++)
    {
        if(arr[n].sector[k].Is_used == 1)
        {
            //cout << arr[i].sector[k].Is_used << " " << arr[i].sector[k].Data << endl;
            Q.push({arr[n].sector[k].Data,{arr[n].sector[k].pre_position, arr[n].sector[k].Is_used}});
            arr[n].sector[k].Data = "";
            arr[n].sector[k].Is_used = false;
            arr[n].sector[k].pre_position = -1;
        }
    }
    for(int i=0; i<ST; i++)
    {
        bool check = false;
        for(int k=0; k<SS; k++)
        {
            if(arr[i].sector[k].Is_used == false) continue;
            check = true;
            break;
        }
        
        // 여기에 프리 블록이 없을 경우 할당하는 프로그램 생성
        
        if(check == false)
        {
            int cnt =0;
            while(!Q.empty())
            {
                //cout << Q.front().first << " " << Q.front().second.first << " " << Q.front().second.second << endl;
                arr[i].sector[cnt].Data = Q.front().first;
                arr[i].sector[cnt].Is_used = Q.front().second.second;
                arr[i].sector[cnt].pre_position = Q.front().second.first+SS;
                
                
                Q.pop();
                cnt++;
            }
        }
    }
    
    
    
    
    
}

void operation_distinction()
{
    while (!used_block.empty())
    {
     
        bool check = false;
        for(int k=1; k<SS; k++)
        {
            if(arr[used_block.front().BI].sector[k].pre_position != arr[used_block.front().BI].sector[k-1].pre_position+1)
            {
                check = true;
            }
        }
        
        if(check == true)
        {
            cout << "merge" << endl;
            merge_operation(used_block.front().BI);
        }
        else if(check == false){
            //cout << "BS : " << i << " OF : " << k << " ";
            cout << "switch" << endl;
            switch_operation(used_block.front().BI);
        }
        
        used_block.pop();
    }
}

int Bast_algorithm(int n, string data)
{
    int BI = 0;
    int OF = 0;
    
    BI = n/SS;
    OF = n%SS;

    if(use_able_log_block.empty())
    {
        operation_distinction();
    }
    
    
    if(arr[BI].log_block == -1)
    {
        arr[BI].log_block = use_able_log_block.top().BI;
        used_block.push(use_able_log_block.top());
        use_able_log_block.pop();
        for(int i=0; i<SS; i++)
        {
            if(arr[arr[BI].log_block].sector[i].Is_used != 0) continue;
            
            arr[arr[BI].log_block].sector[i].Is_used = 1;
            arr[arr[BI].log_block].sector[i].Data = data;
            arr[arr[BI].log_block].sector[i].pre_position = n;
            break;
        }
    }
    else
    {
        bool check = false;
        for(int i=0; i<SS; i++)
        {
            if(arr[arr[BI].log_block].sector[i].Is_used != 0) continue;
            
            check = true;
            arr[arr[BI].log_block].sector[i].Is_used = 1;
            arr[arr[BI].log_block].sector[i].Data = data;
            arr[arr[BI].log_block].sector[i].pre_position = n;
            break;
        }
        
        if(check == false)
        {
            arr[BI].log_block = use_able_log_block.top().BI;
            used_block.push(use_able_log_block.top());
            use_able_log_block.pop();
            for(int i=0; i<SS; i++)
            {
                if(arr[arr[BI].log_block].sector[i].Is_used != 0) continue;
                
                arr[arr[BI].log_block].sector[i].Is_used = 1;
                arr[arr[BI].log_block].sector[i].Data = data;
                arr[arr[BI].log_block].sector[i].pre_position = n;
                break;
            }
        }
    }
    
    
    return 0;
}


void FTL_write(int n, string data)
{
    // i가 lbn mapping[i]가 pbn
    int BI = 0;
    int OF = 0;
    
    BI = n/SS;
    OF = n%SS;
    
    if(arr[BI].sector[OF].Is_used == 1 || arr[BI].sector[OF].Is_used == 2)// overwrite시 실행
    {
        for(int i=0; i<BS; i++)
        {
            for(int k=0; k<SS; k++)
            {
                if(arr[i].sector[k].pre_position == n && arr[i].sector[k].Is_used != 2)
                {
                    arr[i].sector[k].Is_used = 2;
                    break;
                }
            }
        }
        Bast_algorithm(n, data);
    }
    else{
        
        arr[BI].sector[OF].Is_used = 1;
        arr[BI].sector[OF].pre_position = n;
        arr[BI].sector[OF].Data = data;
        
//        for(int i=0; i<ST; i++)
//        {
//            if(Mapping_Table_sector[i] == -1)
//            {
//                cout << "BI : " << BI << endl;
//                Mapping_Table_sector[i] = BI;
//                break;
//            }
//        }
       
    }

    
    for(int i=0; i<ST; i++)
    {
        //cout << Mapping_Table_sector[i] << endl;
    }
}

void FTL_read()
{
    
    for(int i=0; i<BS; i++)
    {
        for(int k=0; k< SS; k++)
        {
            if(arr[i].sector[k].Is_used == false)
            {
                cout << "BS : " << i << " " << "OF : " << k << " empty!" << endl;
            }
            else{
                cout << arr[i].sector[k].Data << " "<< arr[i].sector[k].pre_position <<" "<< arr[i].sector[k].Is_used<< endl;
            }
        }
        cout << endl;
    }
    

}


int main(int argc, const char * argv[]) {
    
    memset(Mapping_Table_sector, -1 , sizeof(int) * ST);
    Make_Log_Block();
    
    /*FTL_write(5,"A");
    FTL_write(5,"B");
    FTL_write(5,"C");
    FTL_write(5,"D");
    FTL_write(5,"king");
    FTL_write(5,"queen");
    
    FTL_write(15,"z");
    FTL_write(15,"x");
    FTL_write(15,"y");
    FTL_write(15,"t");
    FTL_write(15,"s");
    */
//    FTL_write(4, "qwe");
//    FTL_write(5, "qwe");
//    FTL_write(6, "qwe");
//    FTL_write(7, "qwe");
//
//    FTL_write(4, "zzzzzzzz");
//    //FTL_write(1, "qwe");
//    //FTL_write(2, "qwe");
//    //FTL_write(3, "qwe");
//
//    FTL_write(0, "A");
//    FTL_write(0, "V");
    
    
    //FTL_write(15,"123");
    
    int cnt =0;

    FTL_write(0, "1");
    FTL_write(1, "2");
    FTL_write(2, "3");
    FTL_write(3, "4");
    
    FTL_write(0, "A");
    FTL_write(2, "B");
   
    //FTL_write(0, "4");
    operation_distinction();
    
    //FTL_write(4, "zz");
    
    //merge_operation(<#int n#>)
    //switch_operation();
    FTL_read();
}