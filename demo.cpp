#define _CRT_SECURE_NO_WARNINGS
#include <iostream>
#include <vector>
#include <cassert>
#include <cmath>
#include <map>
#include <algorithm>
#include <cstring>
#include <list>
#include <cstdio>
//#include <windows.h>
#include <string>
#include <unistd.h>
#include <random>
#include <ctime>



using namespace std;

struct type_worktable_struct
{
    int raw_material;//每种类型的工作台需要原材料raw_material
    int period;//需要周期period
    int produce;//生产出了produce
};

struct Point
{
    double x, y;
};

// 工作台
struct Workbench {
    int type; // 工作台类型
    Point pos; // 坐标
    int remaining_time; // 剩余生产时间（帧数）
    int raw_bits; // 原材料格状态，二进制位表描述，例如 48（110000）表示拥有物品 4 和 5
    int product_bit; // 产品格状态，0 表示无，1 表示有
    int lock;//当前控制台是否有机器人奔向
};

// 机器人
struct Robot {
    int workbench_id; // 所处工作台 ID，-1 表示当前没有处于任何工作台附近，[0,工作台总数-1]表示某工作台的下标，从0开始，按输入顺序定。当前机器人的所有购买、出售行为均针对该工作台进行
    int carrying_type; // 携带物品类型，0 表示未携带物品，1-7 表示对应物品
    double time_value_coef; // 时间价值系数，携带物品时为 [0.8,1] 的浮点数，不携带物品时为 0
    double collision_value_coef; // 碰撞价值系数，携带物品时为 [0.8,1] 的浮点数，不携带物品时为 0
    double angular_speed; // 角速度，单位为弧度/秒，正数表示逆时针，负数表示顺时针
    Point linear_speed; // 线速度，由二维向量描述线速度，单位为米/秒，每秒有50帧
    double facing_direction; // 朝向，表示机器人的朝向，范围为 [-π,π]，方向示例：0 表示右方向，π/2 表示上方向，-π/2 表示下方向
    Point pos; // 坐标
    Point before_pos;
    int buy;//准备购买
    int sell;//准备出售
    int destroy;//
    int table_id;//前往的工作台ID
};

// 读取一个 Point 类型的变量
Point read_point() {
    Point point;
    cin >> point.x >> point.y;
    return point;
};

//全局变量定义部分
double const pi = 3.1415926535;
char mp[105][105];
int workbench_cnt; // 场上工作台的数量
int frameID, money;
type_worktable_struct tp_worktable[10];
Workbench workbenches[55];//为了通过工作台ID访问工作台
Robot robots[4];//为了通过机器人ID访问机器人

// 生成范围为[min, max]的随机浮点数
double random_double(double min, double max) {
    static std::mt19937 generator(std::time(0));
    std::uniform_real_distribution<double> distribution(min, max);
    return distribution(generator);
}
// 读取一帧的信息
void read_frame_info(int& money) {
    cin >> money >> workbench_cnt;
    for (int i = 0; i < workbench_cnt; i++) {
        cin >> workbenches[i].type;
        workbenches[i].pos = read_point();
        cin >> workbenches[i].remaining_time >> workbenches[i].raw_bits >> workbenches[i].product_bit;
    }
    for (int i = 0; i < 4; i++) {
        cin >> robots[i].workbench_id >> robots[i].carrying_type >> robots[i].time_value_coef >> robots[i].collision_value_coef;
        // 读入机器人的携带物品的时间价值系数和碰撞价值系数
        // 读入机器人的角速度、线速度、朝向和位置

        if (frameID % 300 == 0) {
            robots[i].before_pos.x = robots[i].pos.x;
            robots[i].before_pos.y = robots[i].pos.y;
        }

        cin >> robots[i].angular_speed >> robots[i].linear_speed.x >> robots[i].linear_speed.y>> robots[i].facing_direction >> robots[i].pos.x >> robots[i].pos.y;
    }
    // 读入一行字符串，判断是否输入完毕
    string ok;
    cin >> ok;
    assert(ok == "OK"); // 最后一行必须是OK
}

void init()//初始化每种类型的工作台的信息
{
    tp_worktable[1].period = 50;
    tp_worktable[1].raw_material = 0;
    tp_worktable[1].produce = 1;

    tp_worktable[2].period = 50;
    tp_worktable[2].raw_material = 0;
    tp_worktable[2].produce = 2;

    tp_worktable[3].period = 50;
    tp_worktable[3].raw_material = 0;
    tp_worktable[3].produce = 3;

    tp_worktable[4].period = 500;
    tp_worktable[4].raw_material = (1 << 1) | (1 << 2);
    tp_worktable[4].produce = 4;

    tp_worktable[5].period = 500;
    tp_worktable[5].raw_material = (1 << 1) | (1 << 3);
    tp_worktable[5].produce = 5;

    tp_worktable[6].period = 500;
    tp_worktable[6].raw_material = (1 << 2) | (1 << 3);
    tp_worktable[6].produce = 6;

    tp_worktable[7].period = 1000;
    tp_worktable[7].raw_material = (1 << 4) | (1 << 5) | (1 << 6);
    tp_worktable[7].produce = 7;

    tp_worktable[8].period = 1;
    tp_worktable[8].raw_material = (1 << 7);
    tp_worktable[8].produce = 0;

    tp_worktable[9].period = 1;
    tp_worktable[9].raw_material = (1 << 1) | (1 << 2) | (1 << 3)| (1 << 4) | (1 << 5) | (1 << 6) | (1 << 7);
    tp_worktable[9].produce = 0;

}

//载入地图
void readmap() {
        for (int i = 1;i <= 100;++i)
            for (int j = 1;j <= 100;++j)
                cin >> mp[i][j];
        string ok;
        cin >> ok;
        cout << ok << endl;
        fflush(stdout);
        assert(ok == "OK"); // 最后一行必须是OK
}

double my_distance(Point p1, Point p2)//计算两个点之间的坐标
{
    return sqrt((p1.x - p2.x) * (p1.x - p2.x) + (p1.y - p2.y) * (p1.y - p2.y));
}

int best_fit(double& min_dis, int robotid)//寻找当前最适合机器人前往的工作台
{
    int min_id = -1;
    min_dis = 99999.0;

    if(robotid==0){
                if(robots[robotid].carrying_type == 0)//机器人没有携带物品
                {
                    for (int i = 0;i < workbench_cnt;++i)//遍历所有工作台(只要曾经有过成品就会出现在full里)
                    {
                        if (workbenches[i].type!=7&&workbenches[i].type!=6)
                            {continue;}
                        
                        if (workbenches[i].product_bit == 0)//如果没有生产出产品
                            {continue;}
                    
                        if (workbenches[i].lock != robotid + 1 && workbenches[i].lock != 0) //如果有其它机器人奔向，本机器人不去
                            {continue;}

                        double dis = my_distance(robots[robotid].pos, workbenches[i].pos);
                        if (min_dis > dis)  //选取有成品的工作台中距离最小的工作台
                        {
                            min_dis = dis;                //距离
                            min_id = i;      //工作台编号
                            robots[robotid].buy = 1;//这个机器人要买东西了
                            
                        }
                    }

                    if(min_id!=-1)return min_id;

                    for (int i = 0;i < workbench_cnt;++i)//遍历所有工作台(只要曾经有过成品就会出现在full里)
                    {
                        if (workbenches[i].type!=1)
                            {continue;}
                        
                        if (workbenches[i].product_bit == 0)//如果没有生产出产品
                            {continue;}
                    
                        if (workbenches[i].lock != robotid + 1 && workbenches[i].lock != 0) //如果有其它机器人奔向，本机器人不去
                            {continue;}

                        double dis = my_distance(robots[robotid].pos, workbenches[i].pos);
                        if (min_dis > dis)  //选取有成品的工作台中距离最小的工作台
                        {
                            min_dis = dis;                //距离
                            min_id = i;      //工作台编号
                            robots[robotid].buy = 1;//这个机器人要买东西了
                        }
                    }
                    if(min_id!=-1)return min_id;
                }
                else//机器人携带物品
                {   //遍历需要机器人所携带材料的工作台(只要曾经需要过机器人携带的材料，就会在这里面)
                    fprintf(stderr, "robotid=%d have obj %d\n",robotid,robots[robotid].carrying_type);
                    for (int i = 0;i < workbench_cnt;i++)//遍历所有工作台(只要曾经有过成品就会出现在full里)
                    {
                        if (workbenches[i].type!=9&&workbenches[i].type!=8)
                            {continue;}
                        //如果这种类型的工作台不需要这种类型的材料
                        if ((tp_worktable[workbenches[i].type].raw_material & (1 << (robots[robotid].carrying_type))) == 0)
                            {continue;}
                        //如果工作台已经有了这种原材料
                        if ((workbenches[i].raw_bits & (1 << (robots[robotid].carrying_type))) != 0)
                            {continue;}
                        //如果有其它机器人奔向，本机器人不去
                        if (workbenches[i].lock != robotid + 1&& workbenches[i].lock != 0)
                            {continue;}
                        double dis = my_distance(robots[robotid].pos, workbenches[i].pos);
                        if (min_dis > dis)  //寻找需要改材料的工作台中最近的
                        {
                            min_dis = dis;                    //距离
                            min_id = i;          //工作台编号
                            robots[robotid].sell = 1;//这个机器人要卖东西了
                        }
                    }
                    if(min_id!=-1)return min_id;
                    for (int i = 0;i < workbench_cnt;i++)//遍历所有工作台(只要曾经有过成品就会出现在full里)
                    {
                        if (workbenches[i].type!=7&&workbenches[i].type!=5&&workbenches[i].type!=4)
                            {continue;}
                        //如果这种类型的工作台不需要这种类型的材料
                        if ((tp_worktable[workbenches[i].type].raw_material & (1 << (robots[robotid].carrying_type))) == 0)
                            {continue;}
                        //如果工作台已经有了这种原材料
                        if ((workbenches[i].raw_bits & (1 << (robots[robotid].carrying_type))) != 0)
                            {continue;}
                        //如果有其它机器人奔向，本机器人不去
                        if (workbenches[i].lock != robotid + 1&& workbenches[i].lock != 0)
                            {continue;}
                        double dis = my_distance(robots[robotid].pos, workbenches[i].pos);
                        if (min_dis > dis)  //寻找需要改材料的工作台中最近的
                        {
                            min_dis = dis;                    //距离
                            min_id = i;          //工作台编号
                            robots[robotid].sell = 1;//这个机器人要卖东西了
                        }
                    }
                    if(min_id!=-1)return min_id;

                    }
                }
    if(robotid==1){
                if(robots[robotid].carrying_type == 0)//机器人没有携带物品
                {
                    for (int i = 0;i < workbench_cnt;++i)//遍历所有工作台(只要曾经有过成品就会出现在full里)
                    {
                        if (workbenches[i].type!=7&&workbenches[i].type!=5)
                            {continue;}
                        
                        if (workbenches[i].product_bit == 0)//如果没有生产出产品
                            {continue;}
                    
                        if (workbenches[i].lock != robotid + 1 && workbenches[i].lock != 0) //如果有其它机器人奔向，本机器人不去
                            {continue;}

                        double dis = my_distance(robots[robotid].pos, workbenches[i].pos);
                        if (min_dis > dis)  //选取有成品的工作台中距离最小的工作台
                        {
                            min_dis = dis;                //距离
                            min_id = i;      //工作台编号
                            robots[robotid].buy = 1;//这个机器人要买东西了
                        }
                    }

                    if(min_id!=-1)return min_id;
                    
                    for (int i = 0;i < workbench_cnt;++i)//遍历所有工作台(只要曾经有过成品就会出现在full里)
                    {
                        if (workbenches[i].type!=2)
                            {continue;}
                        
                        if (workbenches[i].product_bit == 0)//如果没有生产出产品
                            {continue;}
                    
                        if (workbenches[i].lock != robotid + 1 && workbenches[i].lock != 0) //如果有其它机器人奔向，本机器人不去
                            {continue;}

                        double dis = my_distance(robots[robotid].pos, workbenches[i].pos);
                        if (min_dis > dis)  //选取有成品的工作台中距离最小的工作台
                        {
                            min_dis = dis;                //距离
                            min_id = i;      //工作台编号
                            robots[robotid].buy = 1;//这个机器人要买东西了
                        }
                    }

                    if(min_id!=-1)return min_id;
                }
                else//机器人携带物品
                {   //遍历需要机器人所携带材料的工作台(只要曾经需要过机器人携带的材料，就会在这里面)
                    fprintf(stderr, "robotid=%d have obj %d\n",robotid,robots[robotid].carrying_type);
                    for (int i = 0;i < workbench_cnt;i++)//遍历所有工作台(只要曾经有过成品就会出现在full里)
                    {
                        if (workbenches[i].type!=9&&workbenches[i].type!=8)
                            {continue;}
                        //如果这种类型的工作台不需要这种类型的材料
                        if ((tp_worktable[workbenches[i].type].raw_material & (1 << (robots[robotid].carrying_type))) == 0)
                            {continue;}
                        //如果工作台已经有了这种原材料
                        if ((workbenches[i].raw_bits & (1 << (robots[robotid].carrying_type))) != 0)
                            {continue;}
                        //如果有其它机器人奔向，本机器人不去
                        if (workbenches[i].lock != robotid + 1&& workbenches[i].lock != 0)
                            {continue;}
                        double dis = my_distance(robots[robotid].pos, workbenches[i].pos);
                        if (min_dis > dis)  //寻找需要改材料的工作台中最近的
                        {
                            min_dis = dis;                    //距离
                            min_id = i;          //工作台编号
                            robots[robotid].sell = 1;//这个机器人要卖东西了
                        }
                    }
                    if(min_id!=-1)return min_id;
                    for (int i = 0;i < workbench_cnt;i++)//遍历所有工作台(只要曾经有过成品就会出现在full里)
                    {
                        if (workbenches[i].type!=7&&workbenches[i].type!=6&&workbenches[i].type!=4)
                            {continue;}
                        //如果这种类型的工作台不需要这种类型的材料
                        if ((tp_worktable[workbenches[i].type].raw_material & (1 << (robots[robotid].carrying_type))) == 0)
                            {continue;}
                        //如果工作台已经有了这种原材料
                        if ((workbenches[i].raw_bits & (1 << (robots[robotid].carrying_type))) != 0)
                            {continue;}
                        //如果有其它机器人奔向，本机器人不去
                        if (workbenches[i].lock != robotid + 1&& workbenches[i].lock != 0)
                            {continue;}
                        double dis = my_distance(robots[robotid].pos, workbenches[i].pos);
                        if (min_dis > dis)  //寻找需要改材料的工作台中最近的
                        {
                            min_dis = dis;                    //距离
                            min_id = i;          //工作台编号
                            robots[robotid].sell = 1;//这个机器人要卖东西了
                        }
                    }
                    if(min_id!=-1)return min_id;

                    }
                }
    if(robotid==2){
                if(robots[robotid].carrying_type == 0)//机器人没有携带物品
                {
                    for (int i = 0;i < workbench_cnt;++i)//遍历所有工作台(只要曾经有过成品就会出现在full里)
                    {
                        if (workbenches[i].type!=7&&workbenches[i].type!=4)
                            {continue;}
                        
                        if (workbenches[i].product_bit == 0)//如果没有生产出产品
                            {continue;}
                    
                        if (workbenches[i].lock != robotid + 1 && workbenches[i].lock != 0) //如果有其它机器人奔向，本机器人不去
                            {continue;}

                        double dis = my_distance(robots[robotid].pos, workbenches[i].pos);
                        if (min_dis > dis)  //选取有成品的工作台中距离最小的工作台
                        {
                            min_dis = dis;                //距离
                            min_id = i;      //工作台编号
                            robots[robotid].buy = 1;//这个机器人要买东西了
                        }
                    }
                    if(min_id!=-1)return min_id;
                    for (int i = 0;i < workbench_cnt;++i)//遍历所有工作台(只要曾经有过成品就会出现在full里)
                    {
                        if (workbenches[i].type!=3)
                            {continue;}
                        
                        if (workbenches[i].product_bit == 0)//如果没有生产出产品
                            {continue;}
                    
                        if (workbenches[i].lock != robotid + 1 && workbenches[i].lock != 0) //如果有其它机器人奔向，本机器人不去
                            {continue;}

                        double dis = my_distance(robots[robotid].pos, workbenches[i].pos);
                        if (min_dis > dis)  //选取有成品的工作台中距离最小的工作台
                        {
                            min_dis = dis;                //距离
                            min_id = i;      //工作台编号
                            robots[robotid].buy = 1;//这个机器人要买东西了
                        }
                    }
                    if(min_id!=-1)return min_id;
                }
                else//机器人携带物品
                {   //遍历需要机器人所携带材料的工作台(只要曾经需要过机器人携带的材料，就会在这里面)
                    fprintf(stderr, "robotid=%d have obj %d\n",robotid,robots[robotid].carrying_type);
                    for (int i = 0;i < workbench_cnt;i++)//遍历所有工作台(只要曾经有过成品就会出现在full里)
                    {
                        if (workbenches[i].type!=9&&workbenches[i].type!=8)
                            {continue;}
                        //如果这种类型的工作台不需要这种类型的材料
                        if ((tp_worktable[workbenches[i].type].raw_material & (1 << (robots[robotid].carrying_type))) == 0)
                            {continue;}
                        //如果工作台已经有了这种原材料
                        if ((workbenches[i].raw_bits & (1 << (robots[robotid].carrying_type))) != 0)
                            {continue;}
                        //如果有其它机器人奔向，本机器人不去
                        if (workbenches[i].lock != robotid + 1&& workbenches[i].lock != 0)
                            {continue;}
                        double dis = my_distance(robots[robotid].pos, workbenches[i].pos);
                        if (min_dis > dis)  //寻找需要改材料的工作台中最近的
                        {
                            min_dis = dis;                    //距离
                            min_id = i;          //工作台编号
                            robots[robotid].sell = 1;//这个机器人要卖东西了 
                        }
                    }
                    if(min_id!=-1)return min_id;

                    for (int i = 0;i < workbench_cnt;i++)//遍历所有工作台(只要曾经有过成品就会出现在full里)
                    {
                        if (workbenches[i].type!=7&&workbenches[i].type!=6&&workbenches[i].type!=5)
                            {continue;}
                        //如果这种类型的工作台不需要这种类型的材料
                        if ((tp_worktable[workbenches[i].type].raw_material & (1 << (robots[robotid].carrying_type))) == 0)
                            {continue;}
                        //如果工作台已经有了这种原材料
                        if ((workbenches[i].raw_bits & (1 << (robots[robotid].carrying_type))) != 0)
                            {continue;}
                        //如果有其它机器人奔向，本机器人不去
                        if (workbenches[i].lock != robotid + 1&& workbenches[i].lock != 0)
                            {continue;}
                        double dis = my_distance(robots[robotid].pos, workbenches[i].pos);
                        if (min_dis > dis)  //寻找需要改材料的工作台中最近的
                        {
                            min_dis = dis;                    //距离
                            min_id = i;          //工作台编号
                            robots[robotid].sell = 1;//这个机器人要卖东西了
                        }
                    }
                    if(min_id!=-1)return min_id;


                    }
                }
    if(robotid==3){
                if(robots[robotid].carrying_type == 0)//机器人没有携带物品
                {
                    for (int i = 0;i < workbench_cnt;++i)//遍历所有工作台(只要曾经有过成品就会出现在full里)
                    {
                        if (workbenches[i].type!=7&&workbenches[i].type!=6&&workbenches[i].type!=5)
                            {continue;}
                        
                        if (workbenches[i].product_bit == 0)//如果没有生产出产品
                            {continue;}
                    
                        if (workbenches[i].lock != robotid + 1 && workbenches[i].lock != 0) //如果有其它机器人奔向，本机器人不去
                            {continue;}

                        double dis = my_distance(robots[robotid].pos, workbenches[i].pos);
                        if (min_dis > dis)  //选取有成品的工作台中距离最小的工作台
                        {
                            min_dis = dis;                //距离
                            min_id = i;      //工作台编号
                            robots[robotid].buy = 1;//这个机器人要买东西了
                        }
                    }
                    if(min_id!=-1)return min_id;
                    for (int i = 0;i < workbench_cnt;++i)//遍历所有工作台(只要曾经有过成品就会出现在full里)
                    {
                        if (workbenches[i].type!=3&&workbenches[i].type!=2&&workbenches[i].type!=1)
                            {continue;}
                        
                        if (workbenches[i].product_bit == 0)//如果没有生产出产品
                            {continue;}
                    
                        if (workbenches[i].lock != robotid + 1 && workbenches[i].lock != 0) //如果有其它机器人奔向，本机器人不去
                            {continue;}

                        double dis = my_distance(robots[robotid].pos, workbenches[i].pos);
                        if (min_dis > dis)  //选取有成品的工作台中距离最小的工作台
                        {
                            min_dis = dis;                //距离
                            min_id = i;      //工作台编号
                            robots[robotid].buy = 1;//这个机器人要买东西了
                        }
                    }
                    if(min_id!=-1)return min_id;
                }
                else//机器人携带物品
                {   //遍历需要机器人所携带材料的工作台(只要曾经需要过机器人携带的材料，就会在这里面)
                    fprintf(stderr, "robotid=%d have obj %d\n",robotid,robots[robotid].carrying_type);
                    for (int i = 0;i < workbench_cnt;i++)//遍历所有工作台(只要曾经有过成品就会出现在full里)
                    {
                        if (workbenches[i].type!=8&&workbenches[i].type!=9)
                            {continue;}
                        //如果这种类型的工作台不需要这种类型的材料
                        if ((tp_worktable[workbenches[i].type].raw_material & (1 << (robots[robotid].carrying_type))) == 0)
                            {continue;}
                        //如果工作台已经有了这种原材料
                        if ((workbenches[i].raw_bits & (1 << (robots[robotid].carrying_type))) != 0)
                            {continue;}
                        //如果有其它机器人奔向，本机器人不去
                        if (workbenches[i].lock != robotid + 1&& workbenches[i].lock != 0)
                            {continue;}
                        double dis = my_distance(robots[robotid].pos, workbenches[i].pos);
                        if (min_dis > dis)  //寻找需要改材料的工作台中最近的
                        {
                            min_dis = dis;                    //距离
                            min_id = i;          //工作台编号
                            robots[robotid].sell = 1;//这个机器人要卖东西了
                        }
                    }
                    if(min_id!=-1)return min_id;
                    for (int i = 0;i < workbench_cnt;i++)//遍历所有工作台(只要曾经有过成品就会出现在full里)
                    {
                        if (workbenches[i].type!=7&&workbenches[i].type!=6&&workbenches[i].type!=5)
                            {continue;}
                        //如果这种类型的工作台不需要这种类型的材料
                        if ((tp_worktable[workbenches[i].type].raw_material & (1 << (robots[robotid].carrying_type))) == 0)
                            {continue;}
                        //如果工作台已经有了这种原材料
                        if ((workbenches[i].raw_bits & (1 << (robots[robotid].carrying_type))) != 0)
                            {continue;}
                        //如果有其它机器人奔向，本机器人不去
                        if (workbenches[i].lock != robotid + 1&& workbenches[i].lock != 0)
                            {continue;}
                        double dis = my_distance(robots[robotid].pos, workbenches[i].pos);
                        if (min_dis > dis)  //寻找需要改材料的工作台中最近的
                        {
                            min_dis = dis;                    //距离
                            min_id = i;          //工作台编号
                            robots[robotid].sell = 1;//这个机器人要卖东西了
                        }
                    }
                    if(min_id!=-1)return min_id;
                    }
                }
    return min_id;
}

double cal_angle(int table_id, int robot_id) {
    double dx = workbenches[table_id].pos.x - robots[robot_id].pos.x;
    double dy = workbenches[table_id].pos.y - robots[robot_id].pos.y;
    double angle = atan2(dy, dx) - robots[robot_id].facing_direction;
    if (angle > pi) angle -= 2 * pi;
    if (angle < -pi) angle += 2 * pi;
    return angle;
}

int main() {
    init();
    readmap();
    while (scanf("%d", &frameID) != EOF) {
        read_frame_info(money);
        printf("%d\n", frameID);
        fflush(stdout);
        //update_workbench();
        double lineSpeed = 3;
        double angleSpeed = 1.5;
        double distance;


        for (int robotId = 0; robotId < 4; robotId++) {

            if((robots[robotId].sell==0) && (robots[robotId].buy==0)){
                robots[robotId].table_id = best_fit(distance, robotId);  //只有当机器人需要进行购买或者出售的时候后才去fit
                fprintf(stderr, "robotId=%d go to table=%d with type %d buy=%d  sell=%d\n",robotId, robots[robotId].table_id,workbenches[robots[robotId].table_id].type,robots[robotId].buy,robots[robotId].sell);
                if(robots[robotId].table_id==-1 && robots[robotId].carrying_type != 0 && frameID %500 ==0){
                                    printf("destroy %d\n", robotId);
                                    fprintf(stderr,"robots %d destroy WITH ERROR\n",robotId);   
                }

            }
            //sleep(1);


            if (abs(robots[robotId].before_pos.x-robots[robotId].pos.x)<0.005 && abs(robots[robotId].before_pos.y-robots[robotId].pos.y)<0.005 )
            {
                //robots[robotId].destroy=1;
            }
            
            if (robots[robotId].table_id != -1)
            {
                workbenches[robots[robotId].table_id].lock = robotId+1;//锁
                double move_distance = distance / (1.0 / 50);//线速度
                double rotate_angle = cal_angle(robots[robotId].table_id, robotId) / (1.0 / 50);//加速度
                if (move_distance > 6.0) {
                    move_distance = 6.0;
                }
                if (move_distance < -2.0) {
                    move_distance = -2.0;
                }
                if (rotate_angle > M_PI) {
                    rotate_angle = M_PI;
                }
                if (rotate_angle < -1 * M_PI) {
                    rotate_angle = -1 * M_PI;
                }
                printf("rotate %d %f\n", robotId, rotate_angle);
                fflush(stdout);
                //double random_variation = random_double(-3,0); // 可根据需要自定义范围
                //double modified_move_distance = move_distance + random_variation;


                if(abs(rotate_angle)>1)
                    printf("forward %d 0\n", robotId);
                else if(my_distance(robots[robotId].pos, workbenches[robots[robotId].table_id].pos)<1)
                    printf("forward %d 1\n", robotId);
                else
                    printf("forward %d %f\n", robotId, move_distance);
                //fprintf(stderr,"forward %d %f rotate %f\n", robotId, move_distance,rotate_angle);
                fflush(stdout);
                
                if (robots[robotId].workbench_id == robots[robotId].table_id && robots[robotId].buy==1) {
                printf("buy %d\n", robotId);
                robots[robotId].buy=0;
                workbenches[robots[robotId].table_id].lock =0;//解锁
                fflush(stdout);
                 }
                else if(robots[robotId].destroy==1 && robots[robotId].sell==1){
                    printf("destroy %d\n", robotId);
                    fprintf(stderr,"robots %d destroy\n",robotId);
                    robots[robotId].sell=0;
                    robots[robotId].destroy=0;
                    workbenches[robots[robotId].table_id].lock = 0;//解锁
                    fflush(stdout);
                }
                else if (robots[robotId].workbench_id == robots[robotId].table_id && robots[robotId].sell==1) {
                printf("sell %d\n", robotId);
                robots[robotId].sell=0;
                workbenches[robots[robotId].table_id].lock = 0;//解锁
                fflush(stdout);
                 }
            }





        }
        printf("OK\n");
        fflush(stdout);
    }

    return 0;
}
