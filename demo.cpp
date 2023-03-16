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
#include <string>
#include <random>
#include <ctime>
#include <unistd.h>
//#include <windows.h>
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
    int material_bits; // 原材料格状态，二进制位表描述，例如 48（110000）表示拥有物品 4 和 5
    int product_bit; // 产品格状态，0 表示无，1 表示有
    int product_lock;//当前产品是否有机器人奔向
    int material_lock;//当前材料格是否有机器人奔向
    int material_count;//统计材料格满了多少个
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
    int buy;//准备购买
    int sell;//准备出售
    int destroy;//
    int table_id;//前往的工作台ID
};

//全局变量定义部分
double const pi = 3.1415926535;
char mp[105][105];
int workbench_cnt; // 场上工作台的数量
int frameID, money;
type_worktable_struct tp_worktable[10];
Workbench workbenches[55];//为了通过工作台ID访问工作台
Robot robots[4];//为了通过机器人ID访问机器人

// 读取一个 Point 类型的变量
Point read_point() {
    Point point;
    cin >> point.x >> point.y;
    return point;
};

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
    tp_worktable[9].raw_material = (1 << 1) | (1 << 2) | (1 << 3) | (1 << 4) | (1 << 5) | (1 << 6) | (1 << 7);
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

//第一个参数：0表示原材料，1表示产品。第二个是产品或原材料id
void lock(int material_or_product, int material_or_product_id, int robotid, int workbenchid)//加锁
{
    if (workbenches[workbenchid].type == 8 || workbenches[workbenchid].type == 9)
        return;
    if (material_or_product == 0)//如果是原材料
        {
            fprintf(stderr, "before lock material_lock:%d,workbenchid:%d\n", workbenches[workbenchid].material_lock, workbenchid);
            workbenches[workbenchid].material_lock |= 1 << material_or_product_id;//材料锁按位运算，相应材料格置1
            fprintf(stderr, "after lock material_lock:%d,workbenchid:%d\n", workbenches[workbenchid].material_lock, workbenchid);
        }
    else//如果是产品
        {
            workbenches[workbenchid].product_lock = robotid + 1;//产品锁用机器人编号
        }
}

//第一个参数：0表示原材料，1表示产品。第二个是产品或原材料id
void unlock(int material_or_product, int material_or_product_id, int robotid, int workbenchid)
{
    if (workbenches[workbenchid].type == 8 || workbenches[workbenchid].type == 9)
        return;
    if (material_or_product == 0)//如果是原材料
        {
        fprintf(stderr, "before unlock material_lock:%d,workbenchid:%d\n", workbenches[workbenchid].material_lock, workbenchid);
        workbenches[workbenchid].material_lock &= ~(1 << material_or_product_id);//解材料锁，相应材料格置0
        fprintf(stderr, "after unlock material_lock:%d,workbenchid:%d\n", workbenches[workbenchid].material_lock, workbenchid);
        workbenches[workbenchid].material_bits |= 1 << robots[robotid].carrying_type;//这个是在sell的时候，防止main循环中下一个机器人在这一帧没有得到材料格已经被装满的信息
        }
    else//如果是产品
    {
        workbenches[workbenchid].product_lock = 0;//清空产品锁
        workbenches[workbenchid].product_bit = 0;//这个是在buy的时候，防止main循环中下一个机器人在这一帧没有得到材料格已经被装满的信息
    }
}

//第一个参数：0表示原材料，1表示产品。第二个是产品或原材料id
int check_lock(int material_or_product, int material_or_product_id, int robotid, int workbenchid)//1表示被锁了，0表示没有被锁
{
    if (workbenches[workbenchid].type == 8 || workbenches[workbenchid].type == 9)
        return 0;
    if (material_or_product == 0)//如果是原材料
    {
        if ((workbenches[workbenchid].material_lock & (1 << material_or_product_id)) != 0)//如果材料锁相应位为1
            {
                    return 1;//被其它机器人锁了
            }
    }
    else//如果是产品
    {
        if (workbenches[workbenchid].product_lock != 0 && workbenches[workbenchid].product_lock != robotid + 1)//如果产品锁被锁，而且不是自己锁的
            {
                return 1;//被其它机器人锁了
            }
    }
    return 0;//没有被锁
}

// 生成范围为[min, max]的随机浮点数
double random_double(double min, double max) {
    static std::mt19937 generator(std::time(0));
    std::uniform_real_distribution<double> distribution(min, max);
    return distribution(generator);
}

void init_material_count(int workbenchid)
{
    workbenches[workbenchid].material_count = 0;//先清空
    for (int i = 1;i <= 7;i++)//总共7种材料
        {
            if ((workbenches[workbenchid].material_bits & (1 << i))!=0)//如果材料格有材料
                workbenches[workbenchid].material_count++;//被占用的材料格增加数量
        }
}

// 读取一帧的信息
void read_frame_info(int& money) {
    cin >> money >> workbench_cnt;
    for (int i = 0; i < workbench_cnt; i++) {
        cin >> workbenches[i].type;
        workbenches[i].pos = read_point();
        cin >> workbenches[i].remaining_time >> workbenches[i].material_bits >> workbenches[i].product_bit;
        init_material_count(i);//对material_count进行初始化
    }
    for (int i = 0; i < 4; i++) {
        cin >> robots[i].workbench_id >> robots[i].carrying_type >> robots[i].time_value_coef >> robots[i].collision_value_coef;        // 读入机器人的携带物品的时间价值系数和碰撞价值系数
        cin >> robots[i].angular_speed >> robots[i].linear_speed.x >> robots[i].linear_speed.y >> robots[i].facing_direction >> robots[i].pos.x >> robots[i].pos.y;
        // 读入机器人的角速度、线速度、朝向和位置
    }
    string ok;    // 读入一行字符串，判断是否输入完毕
    cin >> ok;
    assert(ok == "OK"); // 最后一行必须是OK
}

double my_distance(Point p1, Point p2)//计算两个点之间的坐标
{
    return sqrt((p1.x - p2.x) * (p1.x - p2.x) + (p1.y - p2.y) * (p1.y - p2.y));
}

int make_choice(double& min_dis, int robotid, int first_buy_1, int first_buy_2, int first_buy_3, int first_buy_4,
                int second_buy_1, int second_buy_2, int second_buy_3, int first_sell_1, int first_sell_2, int second_sell_1, int second_sell_2, int second_sell_3, int second_sell_4) {
    int min_id = -1;
    int max_material_count=0;
    if (robots[robotid].carrying_type == 0)//机器人没有携带物品
        {
        for (int i = 0;i < workbench_cnt;++i)//遍历所有工作台(只要曾经有过成品就会出现在full里)
            {
            if (workbenches[i].type != first_buy_1 && workbenches[i].type != first_buy_2 && workbenches[i].type != first_buy_3 && workbenches[i].type != first_buy_4)
            {
                continue;
            }

            if (workbenches[i].product_bit == 0)//如果没有生产出产品
                {
                continue;
                }

            //if (workbenches[i].lock != robotid + 1 && workbenches[i].lock != 0) //如果有其它机器人奔向，本机器人不去
            //{continue;}
            if (check_lock(1, tp_worktable[workbenches[i].type].produce, robotid, i))//如果被锁了continue
                {
                continue;
                }
            int need = 0;//这里把need赋值为0
            for (int j = 0;j < workbench_cnt;++j)//遍历所有工作台(只要曾经有过成品就会出现在full里)
                {
                //如果这种类型的工作台不需要这种类型的材料
                if ((tp_worktable[workbenches[j].type].raw_material & (1 << (tp_worktable[workbenches[i].type].produce))) == 0)
                {
                    continue;
                }
                //如果工作台已经有了这种原材料
                if ((workbenches[j].material_bits & (1 << (tp_worktable[workbenches[i].type].produce))) != 0)
                {
                    continue;
                }
                //如果工作台已经的这种原材料已经被死锁了
                if ((workbenches[j].material_lock & (1 << (tp_worktable[workbenches[i].type].produce))) != 0)
                {
                    continue;
                }
                need = 1;
                }
            if (need == 0) continue;//继续寻找下一个工作台，而不是break不找所有工作台

            double dis = my_distance(robots[robotid].pos, workbenches[i].pos);
            /*if (min_dis > dis&& max_material_count <=workbenches[i].material_count)  //选取有成品的工作台中距离最小的工作台
                {
                min_dis = dis;                //距离
                min_id = i;      //工作台编号
                robots[robotid].buy = 1;//这个机器人要买东西了
                max_material_count = workbenches[i].material_count;//更新数量最多的
                }*/
            if(max_material_count < workbenches[i].material_count  || (max_material_count == workbenches[i].material_count && min_dis > dis)){
                min_dis = dis;                //距离
                min_id = i;      //工作台编号
                robots[robotid].buy = 1;//这个机器人要买东西了
                max_material_count = workbenches[i].material_count;//更新数量最多的
            }
            /*if(min_dis > dis){
                min_dis = dis;                //距离
                min_id = i;      //工作台编号
                robots[robotid].buy = 1;//这个机器人要买东西了
                max_material_count = workbenches[i].material_count;//更新数量最多的
            }*/
            }
        if (min_id != -1) {
            lock(1, tp_worktable[workbenches[min_id].type].produce, robotid, min_id);//机器人找到了生产出的产品，需要加产品锁
            return min_id;
        }

        for (int i = 0;i < workbench_cnt;++i)//遍历所有工作台(只要曾经有过成品就会出现在full里)
            {
            if (workbenches[i].type != second_buy_1 && workbenches[i].type != second_buy_2 && workbenches[i].type != second_buy_3)
            {
                continue;
            }

            if (workbenches[i].product_bit == 0)//如果没有生产出产品
                {
                continue;
                }

            //if (workbenches[i].lock != robotid + 1 && workbenches[i].lock != 0) //如果有其它机器人奔向，本机器人不去
            //  {continue;}

            if (check_lock(1, tp_worktable[workbenches[i].type].produce, robotid, i))
            {
                continue;
            }
            int need = 0;//这里把need赋值为0
            for (int j = 0;j < workbench_cnt;++j)//遍历所有工作台(只要曾经有过成品就会出现在full里)
                {
                //如果这种类型的工作台不需要这种类型的材料
                if ((tp_worktable[workbenches[j].type].raw_material & (1 << (tp_worktable[workbenches[i].type].produce))) == 0)
                {
                    continue;
                }
                //如果工作台已经有了这种原材料
                if ((workbenches[j].material_bits & (1 << (tp_worktable[workbenches[i].type].produce))) != 0)
                {
                    continue;
                }
                //如果工作台已经的这种原材料已经被死锁了
                if ((workbenches[j].material_lock & (1 << (tp_worktable[workbenches[i].type].produce))) != 0)
                {
                    continue;
                }
                need = 1;
                }
            if (need == 0) continue;//继续寻找下一个工作台，而不是break不找所有工作台

            double dis = my_distance(robots[robotid].pos, workbenches[i].pos);
            if(max_material_count < workbenches[i].material_count  || (max_material_count == workbenches[i].material_count && min_dis > dis)){
                min_dis = dis;                //距离
                min_id = i;      //工作台编号
                robots[robotid].buy = 1;//这个机器人要买东西了
                max_material_count = workbenches[i].material_count;//更新数量最多的
            }
            /*if(min_dis > dis){
                min_dis = dis;                //距离
                min_id = i;      //工作台编号
                robots[robotid].buy = 1;//这个机器人要买东西了
                max_material_count = workbenches[i].material_count;//更新数量最多的
            }*/
            }
        if (min_id != -1) {
            lock(1, tp_worktable[workbenches[min_id].type].produce, robotid, min_id);//机器人找到了生产出的产品，需要加产品锁
            return min_id;
        }
        }
    else//机器人携带物品
    {   //遍历需要机器人所携带材料的工作台(只要曾经需要过机器人携带的材料，就会在这里面)
        fprintf(stderr, "robotid=%d have obj %d\n", robotid, robots[robotid].carrying_type);
        for (int i = 0;i < workbench_cnt;i++)//遍历所有工作台(只要曾经有过成品就会出现在full里)
            {
            if (workbenches[i].type != first_sell_1 && workbenches[i].type != first_sell_2)
            {
                continue;
            }
            //如果这种类型的工作台不需要这种类型的材料
            if ((tp_worktable[workbenches[i].type].raw_material & (1 << (robots[robotid].carrying_type))) == 0)
            {
                continue;
            }
            //如果工作台已经有了这种原材料
            if ((workbenches[i].material_bits & (1 << (robots[robotid].carrying_type))) != 0)
            {
                continue;
            }
            //如果有其它机器人奔向，本机器人不去
            //if (workbenches[i].lock != robotid + 1&& workbenches[i].lock != 0)
            //{continue;}
            if (check_lock(0, robots[robotid].carrying_type, robotid, i))
            {
                continue;
            }
            double dis = my_distance(robots[robotid].pos, workbenches[i].pos);
            /*if(min_dis > dis){
                min_dis = dis;                //距离
                min_id = i;      //工作台编号
                robots[robotid].buy = 1;//这个机器人要买东西了
                max_material_count = workbenches[i].material_count;//更新数量最多的
            }*/
            if(max_material_count < workbenches[i].material_count  || (max_material_count == workbenches[i].material_count && min_dis > dis)){
                min_dis = dis;                //距离
                min_id = i;      //工作台编号
                robots[robotid].sell = 1;//这个机器人要买东西了
                max_material_count = workbenches[i].material_count;//更新数量最多的
            }
            }
        if (min_id != -1) {  //加材料锁
            lock(0, robots[robotid].carrying_type, robotid, min_id);//机器人找到了空闲的材料格，需要给这个材料格加材料锁
            return min_id;
        }
        for (int i = 0;i < workbench_cnt;i++)//遍历所有工作台(只要曾经有过成品就会出现在full里)
            {
            if (workbenches[i].type != second_sell_1 && workbenches[i].type != second_sell_2 && workbenches[i].type != second_sell_3 && workbenches[i].type != second_sell_4)
            {
                continue;
            }
            //如果这种类型的工作台不需要这种类型的材料
            if ((tp_worktable[workbenches[i].type].raw_material & (1 << (robots[robotid].carrying_type))) == 0)
            {
                continue;
            }
            //如果工作台已经有了这种原材料
            if ((workbenches[i].material_bits & (1 << (robots[robotid].carrying_type))) != 0)
            {
                continue;
            }
            //如果有其它机器人奔向，本机器人不去

            if (check_lock(0, robots[robotid].carrying_type, robotid, i))
            {
                continue;
            }
            double dis = my_distance(robots[robotid].pos, workbenches[i].pos);
            /*if(min_dis > dis){
                min_dis = dis;                //距离
                min_id = i;      //工作台编号
                robots[robotid].buy = 1;//这个机器人要买东西了
                max_material_count = workbenches[i].material_count;//更新数量最多的
            }*/
            if(max_material_count < workbenches[i].material_count  || (max_material_count == workbenches[i].material_count && min_dis > dis)){
                min_dis = dis;                //距离
                min_id = i;      //工作台编号
                robots[robotid].sell = 1;//这个机器人要买东西了
                max_material_count = workbenches[i].material_count;//更新数量最多的
            }
            }
        if (min_id != -1) {  //加材料锁
            lock(0, robots[robotid].carrying_type, robotid, min_id);//机器人找到了空闲的材料格，需要给这个材料格加材料锁
            return min_id;
        }
    }
    return min_id;
}

int best_fit(double& min_dis, int robotid)//寻找当前最适合机器人前往的工作台
{
    int min_id = -1;
    min_dis = 99999.0;
    if (robotid == 0) min_id = make_choice(min_dis, 0, 7, 6, 5, 5, 3, 3, 3, 5, 4, 7, 6, 8, 9);
    if (robotid == 1) min_id = make_choice(min_dis, 1, 7, 4, 6, 6, 2, 2, 2, 6, 5, 7, 4, 8, 9);
    if (robotid == 2) min_id = make_choice(min_dis, 2, 7, 4, 5, 5, 1, 1, 1, 4, 6, 7, 5, 8, 9);
    if (robotid == 3) min_id = make_choice(min_dis, 3, 7, 6, 5, 4, 3, 2, 1, 6, 8, 7, 4, 8, 9);
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

        double lineSpeed = 3;
        double angleSpeed = 1.5;
        double distance = 1.0;
        for (int robotId = 0; robotId < 4; robotId++) {
            if ((robots[robotId].sell == 0) && (robots[robotId].buy == 0)) {
                robots[robotId].table_id = best_fit(distance, robotId);  //只有当机器人需要进行购买或者出售的时候后才去fit
                    fprintf(stderr, "robotId=%d go to table=%d with type %d buy=%d  sell=%d\n", robotId, robots[robotId].table_id, workbenches[robots[robotId].table_id].type, robots[robotId].buy, robots[robotId].sell);
                if (robots[robotId].table_id == -1 && robots[robotId].carrying_type != 0 && frameID % 5000 == 0) {
                    printf("destroy %d\n", robotId);
                    fprintf(stderr, "robots %d destroy WITH ERROR\n", robotId);
                }

            }
            
            if (robots[robotId].table_id != -1)
            {
                //workbenches[robots[robotId].table_id].lock = robotId+1;//锁
                double move_distance = distance / (1.0 / 50);//线速度
                double rotate_angle = cal_angle(robots[robotId].table_id, robotId) / (1.0 / 50);//加速度
                if (move_distance > 6.0) {
                    move_distance = 6.0;
                }
                if (move_distance < -2.0) {
                    move_distance = -2.0;
                }
                if (rotate_angle > pi) {//M_PI不兼容，数字兼容
                    rotate_angle = pi;
                }
                if (rotate_angle < -1 * pi) {
                    rotate_angle = -1 * pi;
                }
                printf("rotate %d %f\n", robotId, rotate_angle);
                fflush(stdout);
                double random_distance = 2.5 + random_double(-0.8, 0.8);
                if (abs(rotate_angle) > 3)
                    printf("forward %d %f\n", robotId, random_distance);//改这个可以修改转圈圈的大小，可能可以出去
                else if (my_distance(robots[robotId].pos, workbenches[robots[robotId].table_id].pos) < 1)
                        printf("forward %d 2\n", robotId);
                else
                        printf("forward %d %f\n", robotId, move_distance);
                    //fprintf(stderr,"forward %d %f rotate %f\n", robotId, move_distance,rotate_angle);     
                    fflush(stdout);

                    if (robots[robotId].workbench_id == robots[robotId].table_id && robots[robotId].buy == 1) {
                        printf("buy %d\n", robotId);
                        robots[robotId].buy = 0;
                        unlock(1, tp_worktable[workbenches[robots[robotId].table_id].type].produce, robotId, robots[robotId].table_id);//解产品锁
                        //workbenches[robots[robotId].table_id].lock =0;//解锁
                        fflush(stdout);
                    }
                    else if (robots[robotId].destroy == 1 && robots[robotId].sell == 1) {
                        printf("destroy %d\n", robotId);
                        //fprintf(stderr, "robots %d destroy\n", robotId);
                        robots[robotId].sell = 0;
                        robots[robotId].destroy = 0;
                        //workbenches[robots[robotId].table_id].lock = 0;//解锁
                        unlock(0, robots[robotId].carrying_type, robotId, robots[robotId].table_id);//解材料锁
                        fflush(stdout);
                    }
                    else if (robots[robotId].workbench_id == robots[robotId].table_id && robots[robotId].sell == 1) {
                        printf("sell %d\n", robotId);
                        robots[robotId].sell = 0;
                        workbenches[robots[robotId].table_id].material_count++;  //原材料增加
                        //workbenches[robots[robotId].table_id].lock = 0;//解锁
                        unlock(0, robots[robotId].carrying_type, robotId, robots[robotId].table_id);//解材料锁
                        fflush(stdout);
                    }
            }
        }
        printf("OK\n");
        fflush(stdout);
    }
    return 0;
}