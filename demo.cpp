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
//#include <unistd.h>
#include <thread>
#include <chrono>

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
    int be_matched;   //当前的工作台是否被征用去给上层工作台送材料
    map<int, int> match;  //用于记录当前工作台征用的送原材料的工作台，key为材料类型，value为对应征用工作台id
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
    int workbench_to_buy_id;
    int workbench_to_sell_id;
    int adjust_motion_flag;
};

//全局变量定义部分
const double pi = 3.1415926535;
const double map_width = 50.0;
const double map_height = 50.0;
const double time_step = 1.0 / 50;
const double total_duration = 180.00; // 180 seconds
char mp[105][105];
int workbench_cnt; // 场上工作台的数量
int frameID, money;
type_worktable_struct tp_worktable[10];
Workbench workbenches[55];//为了通过工作台ID访问工作台
Robot robots[4];//为了通过机器人ID访问机器人
//map<int,int> match[10];
int have_9;  //是否有工作台9
int have_7;
int collide_id;
int workbench_9_id;
vector<Point> workbench_7;  //记录工作台7的坐标

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
    tp_worktable[9].raw_material = (1 << 4) | (1 << 5) | (1 << 6) | (1 << 7);
    tp_worktable[9].produce = 0;

    /*for(int i = 0; i < 55;i++){
        workbenches[i].up_workbenchId = -1;
    }*/
}

//载入地图
void readmap() {
    for (int i = 100;i >= 1;i--)
        for (int j = 1;j <= 100;++j){
            cin >> mp[i][j];
            if(mp[i][j] == '7'){
                Point p = {(j-1)*0.5+0.25,(i-1)*0.5+0.25};
                workbench_7.push_back(p);
                fprintf(stderr,"reain7\n");
            }
        }
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
        //fprintf(stderr, "before lock material_lock:%d,workbenchid:%d\n", workbenches[workbenchid].material_lock, workbenchid);
        workbenches[workbenchid].material_lock |= 1 << material_or_product_id;//材料锁按位运算，相应材料格置1
        //fprintf(stderr, "after lock material_lock:%d,workbenchid:%d\n", workbenches[workbenchid].material_lock, workbenchid);
        }
    else//如果是产品
    {
        if (material_or_product_id == 1 || material_or_product_id == 2 || material_or_product_id == 3)
        {
            return;
        }

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
        //fprintf(stderr, "before unlock material_lock:%d,workbenchid:%d\n", workbenches[workbenchid].material_lock, workbenchid);
        workbenches[workbenchid].material_lock &= ~(1 << material_or_product_id);//解材料锁，相应材料格置0
        //fprintf(stderr, "after unlock material_lock:%d,workbenchid:%d\n", workbenches[workbenchid].material_lock, workbenchid);
        if (robotid != -1)
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
        if ((workbenches[workbenchid].material_bits & (1 << i)) != 0)//如果材料格有材料
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

double cal_angle(int table_id, int robot_id) {
    double dx = workbenches[table_id].pos.x - robots[robot_id].pos.x;
    double dy = workbenches[table_id].pos.y - robots[robot_id].pos.y;
    double angle = atan2(dy, dx) - robots[robot_id].facing_direction;
    if (angle > pi) angle -= 2 * pi;
    if (angle < -1 * pi) angle += 2 * pi;
    return angle;
}

double cal_time_value_coef(double holding_time, double maxX, double minRate)//持有时间 最大帧数 最小比例
{
    holding_time = holding_time * 50;
    if (holding_time >= maxX)
    {
        return minRate;
    }
    double a1 = 1 - sqrt(1 - (1 - holding_time / maxX) * (1 - holding_time / maxX));
    return a1 * (1 - minRate) + minRate;
}

bool make_choice(int robotid) {
    double max_profit = 999999;
    bool yes_or_no = false;
    robots[robotid].workbench_to_buy_id = -1;
    robots[robotid].workbench_to_sell_id = -1;

    int start = (robotid % 2 == 0) ? 0 : workbench_cnt - 1;
    int end = (robotid % 2 == 0) ? workbench_cnt : -1;
    int step = (robotid % 2 == 0) ? 1 : -1;

    for (int i = start; i != end; i += step)//遍历所有工作台(只要曾经有过成品就会出现在full里)
        {
        // 计算利润
        int product_type = workbenches[i].type;
        int product_price;
        int purchase_price;
        switch (product_type) {
            case 1:purchase_price = 5000; product_price = 7000; break;
            case 2:purchase_price = 5000; product_price = 7000; break;
            case 3:purchase_price = 5000; product_price = 7000; break;
            case 4:purchase_price = 15000; product_price = 20000; break;
            case 5:purchase_price = 15000; product_price = 20000; break;
            case 6:purchase_price = 15000; product_price = 20000; break;
            case 7:purchase_price = 76000; product_price = 1050000; break;
            default:purchase_price = 0; product_price = 0; break;
        }
        if(workbenches[i].type == 7 &&((workbenches[i].remaining_time<80&&workbenches[i].remaining_time>0)||workbenches[i].product_bit == 1)&& frameID>8400)
        goto final_time;
        if (workbenches[i].product_bit == 1 ||(workbenches[i].remaining_time<20&&workbenches[i].remaining_time>0))//如果没有生产出产品
            {
            }
        else {continue;}


        if (workbenches[i].type != 7 ) {
            continue;
        }

        if (check_lock(1, tp_worktable[workbenches[i].type].produce, robotid, i))//如果被锁了continue
            {
            continue;
            }
        final_time:
        int need = 0;//这里把need赋值为0
        for (int j = start; j != end; j += step)//遍历所有工作台(只要曾经有过成品就会出现在full里)
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

            if (workbenches[j].remaining_time==-1 ) {
                robots[robotid].workbench_to_buy_id = i;
                robots[robotid].workbench_to_sell_id = j;
                lock(1, tp_worktable[workbenches[robots[robotid].workbench_to_buy_id].type].produce, robotid, robots[robotid].workbench_to_buy_id);//机器人找到了生产出的产品，需要加产品锁
                lock(0, tp_worktable[workbenches[robots[robotid].workbench_to_buy_id].type].produce, robotid, robots[robotid].workbench_to_sell_id);//机器人找到了空闲的材料格，需要给这个材料格加材料锁
                yes_or_no = true;
                return yes_or_no;
            }
            if (workbenches[j].material_count == 2) {
                robots[robotid].workbench_to_buy_id = i;
                robots[robotid].workbench_to_sell_id = j;
                lock(1, tp_worktable[workbenches[robots[robotid].workbench_to_buy_id].type].produce, robotid, robots[robotid].workbench_to_buy_id);//机器人找到了生产出的产品，需要加产品锁
                lock(0, tp_worktable[workbenches[robots[robotid].workbench_to_buy_id].type].produce, robotid, robots[robotid].workbench_to_sell_id);//机器人找到了空闲的材料格，需要给这个材料格加材料锁
                yes_or_no = true;
                return yes_or_no;
            }
            if (workbenches[j].material_count == 1 && workbenches[j].type == 7) {
                robots[robotid].workbench_to_buy_id = i;
                robots[robotid].workbench_to_sell_id = j;
                lock(1, tp_worktable[workbenches[robots[robotid].workbench_to_buy_id].type].produce, robotid, robots[robotid].workbench_to_buy_id);//机器人找到了生产出的产品，需要加产品锁
                lock(0, tp_worktable[workbenches[robots[robotid].workbench_to_buy_id].type].produce, robotid, robots[robotid].workbench_to_sell_id);//机器人找到了空闲的材料格，需要给这个材料格加材料锁
                yes_or_no = true;
                return yes_or_no;
            }
            need = 1;
            double to_buy_time = my_distance(robots[robotid].pos, workbenches[i].pos);
            double to_sell_time = my_distance(workbenches[i].pos, workbenches[j].pos);
            double total_time = to_buy_time + to_sell_time;
            // product_price = product_price * cal_time_value_coef(to_sell_time, 9000, 0.8) - purchase_price;
            //  double profit = product_price / total_time;
            // 更新最大利润和最佳工作台
            if (total_time < max_profit)
            {
                max_profit = total_time;
                robots[robotid].workbench_to_buy_id = i;
                robots[robotid].workbench_to_sell_id = j;
            }
            }
        if (need == 0) continue;//继续寻找下一个工作台，而不是break不找所有工作台
        }

    for (int i = start; i != end; i += step)//遍历所有工作台(只要曾经有过成品就会出现在full里)
        {
        // 计算利润
        int product_type = workbenches[i].type;
        int product_price;
        int purchase_price;
        switch (product_type) {
            case 1:purchase_price = 5000; product_price = 7000; break;
            case 2:purchase_price = 5000; product_price = 7000; break;
            case 3:purchase_price = 5000; product_price = 7000; break;
            case 4:purchase_price = 15000; product_price = 20000; break;
            case 5:purchase_price = 15000; product_price = 20000; break;
            case 6:purchase_price = 15000; product_price = 20000; break;
            case 7:purchase_price = 76000; product_price = 1050000; break;
            default:purchase_price = 0; product_price = 0; break;
        }


        if (workbenches[i].product_bit == 0)//如果没有生产出产品
            {
            continue;
            }

        if (workbenches[i].type == 7 ) {
            continue;
        }

        if (check_lock(1, tp_worktable[workbenches[i].type].produce, robotid, i))//如果被锁了continue
            {
            continue;
            }

        int need = 0;//这里把need赋值为0
        for (int j = start; j != end; j += step)//遍历所有工作台(只要曾经有过成品就会出现在full里)
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
            if (workbenches[j].material_count == 2) {
                robots[robotid].workbench_to_buy_id = i;
                robots[robotid].workbench_to_sell_id = j;
                lock(1, tp_worktable[workbenches[robots[robotid].workbench_to_buy_id].type].produce, robotid, robots[robotid].workbench_to_buy_id);//机器人找到了生产出的产品，需要加产品锁
                lock(0, tp_worktable[workbenches[robots[robotid].workbench_to_buy_id].type].produce, robotid, robots[robotid].workbench_to_sell_id);//机器人找到了空闲的材料格，需要给这个材料格加材料锁
                yes_or_no = true;
                return yes_or_no;
            }
            if (workbenches[j].material_count == 1 && workbenches[j].type == 7) {
                robots[robotid].workbench_to_buy_id = i;
                robots[robotid].workbench_to_sell_id = j;
                lock(1, tp_worktable[workbenches[robots[robotid].workbench_to_buy_id].type].produce, robotid, robots[robotid].workbench_to_buy_id);//机器人找到了生产出的产品，需要加产品锁
                lock(0, tp_worktable[workbenches[robots[robotid].workbench_to_buy_id].type].produce, robotid, robots[robotid].workbench_to_sell_id);//机器人找到了空闲的材料格，需要给这个材料格加材料锁
                yes_or_no = true;
                return yes_or_no;
            }
            need = 1;
            double to_buy_time = my_distance(robots[robotid].pos, workbenches[i].pos);
            double to_sell_time = my_distance(workbenches[i].pos, workbenches[j].pos);
            double total_time = to_buy_time + to_sell_time;
            // product_price = product_price * cal_time_value_coef(to_sell_time, 9000, 0.8) - purchase_price;
            //  double profit = product_price / total_time;
            // 更新最大利润和最佳工作台
            if (total_time < max_profit)
            {
                max_profit = total_time;
                robots[robotid].workbench_to_buy_id = i;
                robots[robotid].workbench_to_sell_id = j;
            }
            }
        if (need == 0) continue;//继续寻找下一个工作台，而不是break不找所有工作台
        }

    if (max_profit != 999999)
    {
        lock(1, tp_worktable[workbenches[robots[robotid].workbench_to_buy_id].type].produce, robotid, robots[robotid].workbench_to_buy_id);//机器人找到了生产出的产品，需要加产品锁
        lock(0, tp_worktable[workbenches[robots[robotid].workbench_to_buy_id].type].produce, robotid, robots[robotid].workbench_to_sell_id);//机器人找到了空闲的材料格，需要给这个材料格加材料锁
        yes_or_no = true;
        return yes_or_no;
    }

    return yes_or_no;
}

bool is_collision_risk(int robot_id) {
    bool risk = false;
    double predicted_x = robots[robot_id].pos.x + robots[robot_id].linear_speed.x * time_step * 15;
    double predicted_y = robots[robot_id].pos.y + robots[robot_id].linear_speed.y * time_step * 15;
    risk = (predicted_x < 0.6 || predicted_x > map_width - 0.6 || predicted_y < 0.6 || predicted_y > map_height - 0.6);

    if (risk)
    {
        //fprintf(stderr, "risk!!!!\n");
    }
    return risk;
}

double robots_angle(Point p, Robot robot1)//判断离开相撞点要转的角度
{
    double dis = my_distance(p, robot1.pos);
    double angle = asin(1.2 / dis);//最大半径两倍的正弦
    if (angle * 50 > pi)
    {
        return pi;
    }
    return angle * 50;
}

// 计算向量的大小
double VectorLength(double vx, double vy) {
    return sqrt(vx * vx + vy * vy);
}

// 计算两个向量的点积
double DotProduct(double vx1, double vy1, double vx2, double vy2) {
    return vx1 * vx2 + vy1 * vy2;
}

// 计算两个向量的叉积
double CrossProduct(double vx1, double vy1, double vx2, double vy2) {
    return vx1 * vy2 - vx2 * vy1;
}

Point Intersection(Point p1, double angle1, Point p2, double angle2) {
    double vx1 = cos(angle1), vy1 = sin(angle1);
    double vx2 = cos(angle2), vy2 = sin(angle2);
    double a = CrossProduct(vx1, vy1, vx2, vy2);
    //double b = CrossProduct(vx1, vy1, p2.x - p1.x, p2.y - p1.y);
    if (a == 0) {
        return { -1, -1 };
    }
    else {
        double t = CrossProduct(vx2, vy2, p2.x - p1.x, p2.y - p1.y) / a;
        double x = p1.x + vx1 * t;
        double y = p1.y + vy1 * t;
        if (x < 0 || x > 50 || y < 0 || y > 50) {
            return { -1, -1 };
        }
        else {
            return { x, y };
        }
    }
}

// 判断两个机器人是否朝向相反
bool is_facing_same_direction(Robot r1, Robot r2)
{
    double angle_diff = abs(r1.facing_direction - r2.facing_direction);
    return angle_diff > 17 * pi / 18;
}

// 判断两个机器人是否朝向相反
bool point_to_each_other(Robot r1, Robot r2)
{
    double dx = r1.pos.x -r2.pos.x;
    double dy = r1.pos.y - r2.pos.y;
    double angle1 = atan2(dy, dx) - r2.facing_direction;
    if (angle1 > pi) angle1 -= 2 * pi;
    if (angle1 < -1 * pi) angle1 += 2 * pi;

    dx = r2.pos.x - r1.pos.x;
    dy = r2.pos.y - r1.pos.y;
    double angle2 = atan2(dy, dx) - r1.facing_direction;
    if (angle2 > pi) angle2 -= 2 * pi;
    if (angle2 < -1 * pi) angle2 += 2 * pi;

    return abs(angle1) < pi / 18&& abs(angle2) < pi / 18;
}

// 判断两个机器人是否有可能在接下来的很近时间内发生追尾碰撞
bool is_possible_collision(Robot r1, Robot r2)
{
    // 计算两个机器人距离
    double dist = my_distance(r1.pos, r2.pos);
    // 如果距离小于两个机器人半径之和的2倍，并且朝向相近，则有可能会发生追尾碰撞
    return dist < 1.06 && is_facing_same_direction(r1, r2)&&point_to_each_other(r1,r2);
}

double isCollide(double& my_rotate, Robot robot1, Robot robot2) {
    double robots_distance = my_distance(robot1.pos, robot2.pos);//计算两个机器人距离
    double crash_time = 50 * robots_distance / (my_distance(robot1.linear_speed, Point{ 0,0 }) + my_distance(robot2.linear_speed, Point{ 0,0 }));//计算直线距离还有多少帧
    double judge_crash_angle = pi / 3;//判断相撞的夹角范围
    double judge_crash_angle2 = pi / 12;//判断相撞的夹角范围
    double sita = atan2(robot1.pos.y - robot2.pos.y, robot1.pos.x - robot2.pos.x) - robot2.facing_direction;//计算2为了和1相撞还要转多少角度
    //double sita = robot1.facing_direction - robot2.facing_direction;
    //下面是为了让sita处于[-1*pi,pi]
    if (sita > pi)
    {
        sita -= 2 * pi;
    }
    if (sita < -1 * pi)
    {
        sita += 2 * pi;
    }
    double sita2 = atan2(robot2.pos.y - robot1.pos.y, robot2.pos.x - robot1.pos.x) - robot1.facing_direction;//计算1为了和2相撞还要转多少角度
    //下面是为了让sita处于[-1*pi,pi]
    if (sita2 > pi)
    {
        sita2 -= 2 * pi;
    }
    if (sita2 < -1 * pi)
    {
        sita2 += 2 * pi;
    }
    double sita3 = fabs(fabs(robot1.facing_direction - robot2.facing_direction) - pi);//两个机器人方向夹角的差值
    //if (distance < robot1.radius + robot2.radius + 0.5 && (sita > pi / 2 && sita < pi)) {
    Point p = Intersection(robot1.pos, robot1.facing_direction, robot2.pos, robot2.facing_direction);
    if (int(crash_time) < 20 && sita3 <= judge_crash_angle)  //控制反应帧为20帧
        {
        if (-judge_crash_angle < sita && sita < 0 && 0 < sita2 && sita2 < judge_crash_angle)//下面分别对应6张图
            {
            if (-sita < sita2)
            {
                my_rotate = -1 * max(robots_angle(p, robot1), robots_angle(p, robot2));//顺时针转是负的
                return true;
            }
            else
            {
                my_rotate = max(robots_angle(p, robot1), robots_angle(p, robot2));//逆时针转是正的
                return true;
            }
            }
        else if (0 < sita && sita < judge_crash_angle && -judge_crash_angle < sita2 && sita2 < 0)
        {
            if (sita < -sita2)
            {
                my_rotate = max(robots_angle(p, robot1), robots_angle(p, robot2));
                return true;
            }
            else
            {
                my_rotate = -1 * max(robots_angle(p, robot1), robots_angle(p, robot2));
                return true;
            }
        }
        else if (-judge_crash_angle < sita && sita < 0 && -judge_crash_angle < sita2 && sita2 < 0)
        {
            my_rotate = max(robots_angle(p, robot1), robots_angle(p, robot2));
            return true;
        }
        else if (0 < sita && sita < judge_crash_angle && 0 < sita2 && sita2 < judge_crash_angle)
        {
            my_rotate = -1 * max(robots_angle(p, robot1), robots_angle(p, robot2));
            return true;
        }
        if (-judge_crash_angle2 < sita && sita < 0 && 0 < sita2)//下面分别对应6张图
            {
            my_rotate = -1 * max(robots_angle(p, robot1), robots_angle(p, robot2));//顺时针转是负的
            return true;
            }
        else if (0 < sita && sita < judge_crash_angle2 && sita2 < 0)
        {
            my_rotate = max(robots_angle(p, robot1), robots_angle(p, robot2));
            return true;
        }
        else if (-judge_crash_angle2 < sita && sita < 0 && sita2 < 0)
        {
            my_rotate = max(robots_angle(p, robot1), robots_angle(p, robot2));
            return true;
        }
        else if (0 < sita && sita < judge_crash_angle2 && sita2 < judge_crash_angle2)
        {
            my_rotate = -1 * max(robots_angle(p, robot1), robots_angle(p, robot2));
            return true;
        }
        }
    return false;
}
// 避免机器人相互碰撞
int avoidCollide() {
    int collide_id = 0;
    // 遍历每个机器人
    for (int i = 0; i < 4; i++) {
        // 遍历其他机器人
        for (int j = i + 1; j < 4; j++) {
            double rotate = 0;
            // 如果两个机器人碰撞了，就更新它们的位置和速度
            if (isCollide(rotate, robots[i], robots[j])) {
                //fprintf(stderr, "    rotate %f robots_id_i %d robots_is_j %d\n", rotate,i,j);
                //Sleep(1000);
                printf("rotate %d %f\n", j, rotate);//2号机器人转的角度
                fflush(stdout);
                printf("rotate %d %f\n", i, rotate);//1号机器人和2号机器人同向转
                fflush(stdout);
                printf("forward %d 5\n", j);//减速，避免碰撞转不过来
                fflush(stdout);
                printf("forward %d 2\n", i);//减速，避免碰撞转不过来
                fflush(stdout);
                collide_id |= 1 << j;//记录哪些机器人会在这一帧进行计算
            }
            else if (is_possible_collision(robots[i], robots[j]))
            {
                printf("rotate %d pi\n", j);//2号机器人转的角度
                fflush(stdout);
                printf("rotate %d pi\n", i);//1号机器人和2号机器人同向转
                fflush(stdout);
                printf("forward %d -2\n", j);//减速，避免碰撞转不过来
                fflush(stdout);
                printf("forward %d -2\n", i);//减速，避免碰撞转不过来
                fflush(stdout);
                collide_id |= 1 << j;//记录哪些机器人会在这一帧进行计算
                collide_id |= 1 << i;//记录哪些机器人会在这一帧进行计算
            }
        }
    }
    return collide_id;//返回机器人编号
}

double clamp(double value, double min_value, double max_value) {
    return std::max(min_value, std::min(value, max_value));
}

const double max_linear_speed = 6.0;
const double max_angular_speed = pi;



double angle_robot(Point a, Point b) {
    return atan2(b.y - a.y, b.x - a.x);
}
// 计算两点之间的引力
Point attractive_force(Point pos1, Point pos2, double coefficient) {
    Point force = { pos2.x - pos1.x, pos2.y - pos1.y };
    force.x *= coefficient;
    force.y *= coefficient;
    return force;
}

Point artificial_potential_field(int robotId) {
    // 设置参数
    double repulsive_coefficient = 200;  // 斥力系数
    double repulsive_distance = 1.8 * 0.53;  // 斥力作用距离
    double attractive_coefficient = 500;  // 引力系数
    double robot_radius = 0.53;  // 机器人半径
    double exponential_factor = 0;  // 墙壁斥力增长的指数因子

    // 初始化合成势场
    Point synthetic_force = { 0, 0 };

    // 计算机器人与墙壁之间的斥力
    double wall_force_x = repulsive_coefficient * (1 / pow(robots[robotId].pos.x - robot_radius, exponential_factor) - 1 / pow(map_width - robots[robotId].pos.x - robot_radius, exponential_factor));
    double wall_force_y = repulsive_coefficient * (1 / pow(robots[robotId].pos.y - robot_radius, exponential_factor) - 1 / pow(map_height - robots[robotId].pos.y - robot_radius, exponential_factor));
    synthetic_force.x += wall_force_x;
    synthetic_force.y += wall_force_y;

    // 计算机器人之间的斥力
    for (int i = 0; i < 4; i++) {
        if (i != robotId) {
            double distance = my_distance(robots[robotId].pos, robots[i].pos);
            if (distance < repulsive_distance + 2 * robot_radius) {  // 判断机器人之间的距离是否在斥力作用范围内
                // if (robotId > i) { // 只有具有较低优先级的机器人受到斥力
                double repulsive_force = repulsive_coefficient * (1 / (distance - 2 * robot_radius) - 1 / repulsive_distance);
                double angle = angle_robot(robots[i].pos, robots[robotId].pos);  // 修改为从机器人i指向机器人robotId的角度

                // 计算斥力作用在机器人上的分量
                double repulsive_force_x = repulsive_force * cos(angle);
                double repulsive_force_y = repulsive_force * sin(angle);
                ;
                // 累加斥力
                synthetic_force.x += repulsive_force_x;
                synthetic_force.y += repulsive_force_y;
                //  }
            }
        }
    }


    // 计算机器人与工作台之间的引力
    int target_workbench_id = robots[robotId].buy ? robots[robotId].workbench_to_buy_id : robots[robotId].workbench_to_sell_id;
    Point workbench_pos = workbenches[target_workbench_id].pos;
    Point attraction_force = attractive_force(robots[robotId].pos, workbench_pos, attractive_coefficient);
    synthetic_force.x += attraction_force.x;
    synthetic_force.y += attraction_force.y;

    return synthetic_force;
}

// 计算两点之间的角度差
double angle_difference(double current_angle, double target_angle) {
    double diff = target_angle - current_angle;
    if (diff > pi) {
        diff -= 2 * pi;
    }
    else if (diff < -pi) {
        diff += 2 * pi;
    }
    return diff;
}

void shundai(int workbench_id, int robotid)
{
    if (workbenches[robots[robotid].workbench_to_sell_id].product_lock == 0)
    {
        robots[robotid].workbench_to_buy_id = robots[robotid].workbench_to_sell_id;
        robots[robotid].workbench_to_sell_id = workbench_id;
        lock(1, tp_worktable[workbenches[robots[robotid].workbench_to_buy_id].type].produce, robotid, robots[robotid].workbench_to_buy_id);//机器人找到了生产出的产品，需要加产品锁
        lock(0, tp_worktable[workbenches[robots[robotid].workbench_to_buy_id].type].produce, robotid, robots[robotid].workbench_to_sell_id);//机器人找到了空闲的材料格，需要给这个材料格加材料锁
        return;
    }
    int change_robot_id = workbenches[robots[robotid].workbench_to_sell_id].product_lock - 1;//获取当前产品格被哪个机器人锁掉
    fprintf(stderr, "workbench_id %d robotid %d change_robot_id %d\n", workbench_id, robotid, change_robot_id);
    workbenches[robots[robotid].workbench_to_sell_id].product_lock = robotid + 1;//更换机器人锁
    robots[robotid].workbench_to_buy_id = robots[robotid].workbench_to_sell_id;
    robots[robotid].workbench_to_sell_id = robots[change_robot_id].workbench_to_sell_id;
    //不需要再占一个材料锁，因为之前的机器人已经占有了一个材料锁
    //workbenches[robots[change_robot_id].workbench_to_sell_id].material_lock &= ~(1 << (tp_worktable[workbenches[workbench_id].type].produce));//删掉终点的死锁
    fprintf(stderr, "robot_id %d buy %f %f sell %d\n", change_robot_id, robots[change_robot_id].pos.x, robots[change_robot_id].pos.y, robots[change_robot_id].workbench_to_sell_id);
    make_choice(change_robot_id);//重新规划
    robots[change_robot_id].buy = 1;
    robots[change_robot_id].sell = 1;
    fprintf(stderr, "robot_id %d buy %f %f sell %d\n", change_robot_id, robots[change_robot_id].pos.x, robots[change_robot_id].pos.y, robots[change_robot_id].workbench_to_sell_id);
}

auto start_time = std::chrono::steady_clock::now();
void give_command(int robotId, bool collision_risk) {
    double lineSpeed = 3;
    double angleSpeed = 1.5;
    double distance = 1.0;
    double random_distance = 2 + random_double(-0.8, 0.4);


    // 获取人工势场法结果
    Point apf_force = artificial_potential_field(robotId);
    double modified_angle = angle_robot(robots[robotId].pos, { robots[robotId].pos.x + apf_force.x, robots[robotId].pos.y + apf_force.y });

    // 计算角速度
    double angle_difference_to_target = angle_difference(robots[robotId].facing_direction, modified_angle);
    angleSpeed = angle_difference_to_target / time_step;
    angleSpeed = clamp(angleSpeed, -max_angular_speed, max_angular_speed);



    if (robots[robotId].buy == 1 &&frameID<8800)
    {
        double lineSpeed = distance / time_step;//线速度
        //double angleSpeed = cal_angle(robots[robotId].workbench_to_buy_id, robotId) / time_step;//加速度

        lineSpeed = clamp(lineSpeed, -2, 6);
        angleSpeed = clamp(angleSpeed, -1 * pi, pi);

        if (abs(cal_angle(robots[robotId].workbench_to_buy_id, robotId)) > 1.535 && my_distance(robots[robotId].pos, workbenches[robots[robotId].workbench_to_buy_id].pos) < 2.2)
            printf("rotate %d %f\n", robotId, -angleSpeed);
        else
            printf("rotate %d %f\n", robotId, angleSpeed);

        //if (abs(angleSpeed) > 3 && collision_risk)
        ///  printf("forward %d -0.1\n", robotId);
        //else
        if (abs(cal_angle(robots[robotId].workbench_to_buy_id, robotId)) > 1.5 && my_distance(robots[robotId].pos, workbenches[robots[robotId].workbench_to_buy_id].pos) < 1)
            printf("forward %d -1.95\n", robotId);
        else if (collision_risk)
            printf("forward %d 0.94\n", robotId);

        else
            printf("forward %d %f\n", robotId, random_double(5.65, 6));
        fflush(stdout);

        if (robots[robotId].workbench_id == robots[robotId].workbench_to_buy_id && robots[robotId].buy == 1) {
            printf("buy %d\n", robotId);
            robots[robotId].buy = 0;
            unlock(1, tp_worktable[workbenches[robots[robotId].workbench_to_buy_id].type].produce, robotId, robots[robotId].workbench_to_buy_id);//解产品锁
            fflush(stdout);
        }
    }

    else if (robots[robotId].sell == 1)
    {
        double lineSpeed = distance / time_step;//线速度
        //double angleSpeed = cal_angle(robots[robotId].workbench_to_sell_id, robotId) / time_step;//加速度

        lineSpeed = clamp(lineSpeed, -2, 6);
        angleSpeed = clamp(angleSpeed, -1 * pi, pi);

        if (abs(cal_angle(robots[robotId].workbench_to_sell_id, robotId)) > 1.535 && my_distance(robots[robotId].pos, workbenches[robots[robotId].workbench_to_sell_id].pos) < 2.2)
            printf("rotate %d %f\n", robotId, -angleSpeed);
        else
            printf("rotate %d %f\n", robotId, angleSpeed);

        //if (abs(angleSpeed) > 3 && collision_risk)
        //  printf("forward %d -0.1\n", robotId);
        //else
        if (abs(cal_angle(robots[robotId].workbench_to_sell_id, robotId)) > 1.535 && my_distance(robots[robotId].pos, workbenches[robots[robotId].workbench_to_sell_id].pos) < 5)
            printf("forward %d -1.95\n", robotId);
        else  if (collision_risk && workbenches[robots[robotId].workbench_to_sell_id].type != 9)
            printf("forward %d 0.94\n", robotId);
        else
            printf("forward %d %f\n", robotId, random_double(5.65, 6));
        fflush(stdout);

        if (robots[robotId].workbench_id == robots[robotId].workbench_to_sell_id && robots[robotId].sell == 1) {
            printf("sell %d\n", robotId);
            robots[robotId].sell = 0;
            workbenches[robots[robotId].workbench_to_sell_id].material_count++;  //原材料增加

            //std::thread([robotId]() {
            //  std::this_thread::sleep_for(std::chrono::milliseconds(1000));
            unlock(0, robots[robotId].carrying_type, robotId, robots[robotId].workbench_to_sell_id); // 解材料锁


            if (workbenches[robots[robotId].workbench_to_sell_id].type == 7 && workbenches[robots[robotId].workbench_to_sell_id].product_bit == 1&&frameID<8800)
            {
                robots[robotId].workbench_to_buy_id = robots[robotId].workbench_to_sell_id;
                for (int j = 0; j < workbench_cnt; j++)
                {
                    if (workbenches[j].type == 8) {
                        robots[robotId].workbench_to_sell_id = j;
                        break;
                    }
                }
                robots[robotId].buy = 1;
                robots[robotId].sell = 1;
                lock(1, tp_worktable[workbenches[robots[robotId].workbench_to_buy_id].type].produce, robotId, robots[robotId].workbench_to_buy_id);//机器人找到了生产出的产品，需要加产品锁
                lock(0, tp_worktable[workbenches[robots[robotId].workbench_to_buy_id].type].produce, robotId, robots[robotId].workbench_to_sell_id);//机器人找到了空闲的材料格，需要给这个材料格加材料锁
            }
            //}).detach();
            if (have_9 && !have_7 && (workbenches[robots[robotId].workbench_to_sell_id].type == 6 || workbenches[robots[robotId].workbench_to_sell_id].type == 5 || workbenches[robots[robotId].workbench_to_sell_id].type == 4) && workbenches[robots[robotId].workbench_to_sell_id].product_bit == 1&&frameID<8800)
            {
                robots[robotId].workbench_to_buy_id = robots[robotId].workbench_to_sell_id;
                for (int j = 0; j < workbench_cnt; j++)
                {
                    if (workbenches[j].type == 9) {
                        robots[robotId].workbench_to_sell_id = j;
                        break;
                    }
                }
                robots[robotId].buy = 1;
                robots[robotId].sell = 1;
                lock(1, tp_worktable[workbenches[robots[robotId].workbench_to_buy_id].type].produce, robotId, robots[robotId].workbench_to_buy_id);//机器人找到了生产出的产品，需要加产品锁
                lock(0, tp_worktable[workbenches[robots[robotId].workbench_to_buy_id].type].produce, robotId, robots[robotId].workbench_to_sell_id);//机器人找到了空闲的材料格，需要给这个材料格加材料锁
            }
            fflush(stdout);
        }
    }

}


bool judge_pickup(int workbenchId, int& go_workbenchId, int materialId) {
    map<int, int>::iterator it;
    for (it = workbenches[workbenchId].match.begin(); it != workbenches[workbenchId].match.end(); it++) {
        if (it->first != materialId)   //不是所需的材料号
            continue;
        go_workbenchId = it->second;  //直接获取去取改材料的工作台id
        if (workbenches[it->second].product_lock == 0) {  //产品未被锁（1/2/3本身无锁）
            if (workbenches[go_workbenchId].type == 1 || workbenches[go_workbenchId].type == 2 || workbenches[go_workbenchId].type == 3)  //这里是我觉得1/2/3没必要判断有没有产品，反正很快就生产出来了（调试结果，不然机器人就去make_choice了)
                return true;
            if (workbenches[it->second].product_bit == 1) { //如果该工作台有产品且未被锁,即可以取它的产品
                return true;
            }
        }
        else     //原材料工作台没有产品产生或被锁了
        return  false;
    }
}

int find_robot(int go_workbenchId) {
    double dis = 2501.0;
    int robotId = -1;
    for (int i = 0;i < 4; i++) {
        if ((robots[i].sell == 0) && (robots[i].buy == 0)) {  //该机器人空闲
            double new_dis = my_distance(workbenches[go_workbenchId].pos, robots[i].pos);
            if (new_dis < dis) {   //找距离最短的
                robotId = i;
                dis = new_dis;
            }
        }
    }
    return robotId;
}

bool do_choice(int workbenchId, int& go_workbenchId, int materialId) {
    int robotId = -1;
    go_workbenchId = -1;
    if ((workbenches[workbenchId].material_bits & (1 << materialId)) == 0 && check_lock(0, materialId, 0, workbenchId) == 0) {  //缺材料且没被锁
        //lock(0,materialId,0,workbenchId);
        if (judge_pickup(workbenchId, go_workbenchId, materialId)) {     //该平台改材料对应的生产平台是否可取产品
            robotId = find_robot(go_workbenchId);  //找空闲机器人
            if (robotId != -1) {   //找到了
                lock(0, materialId, 0, workbenchId); //上材料锁
                lock(1, materialId, robotId, go_workbenchId); //上产品锁
                robots[robotId].buy = 1;
                robots[robotId].sell = 1;
                robots[robotId].workbench_to_sell_id = workbenchId;
                robots[robotId].workbench_to_buy_id = go_workbenchId;
            }
            return true;
        }
        else {  //暂时还没工作台生产出产品，此时go_workbenchId是匹配的工作台
            return false;
        }
    }
    return true;  //已经有了或者已经在送了
}

void make_choice_sec(int workbenchId) {
    int go_workbenchId = -1;
    if (workbenches[workbenchId].type == 7) {
        if (!do_choice(workbenchId, go_workbenchId, 4)) {
            make_choice_sec(go_workbenchId);   //递归调用
        }
        if (!do_choice(workbenchId, go_workbenchId, 5)) {
            make_choice_sec(go_workbenchId);   //递归调用
        }
        if (!do_choice(workbenchId, go_workbenchId, 6)) {
            make_choice_sec(go_workbenchId);   //递归调用
        }
    }
    else if (workbenches[workbenchId].type == 4) {
        if (!do_choice(workbenchId, go_workbenchId, 1)) {
            make_choice_sec(go_workbenchId);   //递归调用
        }
        if (!do_choice(workbenchId, go_workbenchId, 2)) {
            make_choice_sec(go_workbenchId);   //递归调用
        }
    }
    else if (workbenches[workbenchId].type == 5) {
        if (!do_choice(workbenchId, go_workbenchId, 1)) {
            make_choice_sec(go_workbenchId);   //递归调用
        }
        if (!do_choice(workbenchId, go_workbenchId, 3)) {
            make_choice_sec(go_workbenchId);   //递归调用
        }
    }
    else if (workbenches[workbenchId].type == 6) {
        if (!do_choice(workbenchId, go_workbenchId, 2)) {
            make_choice_sec(go_workbenchId);   //递归调用
        }
        if (!do_choice(workbenchId, go_workbenchId, 3)) {
            make_choice_sec(go_workbenchId);   //递归调用
        }
    }
    return;
}

bool compare_pos(Point pos1,vector<Point>::iterator pos2){
    if(pos1.x==pos2->x && pos1.y==pos2->y)
        return true;
    else
        return false;
}

void plan_strategy() {
    for (int i = 0;i < workbench_cnt;i++) {  //遍历所有工作台
        double dis1 = 2501.0, dis2 = 2501.0, dis3 = 2501.0;  //单纯因为不想遍历多次，比如针对7，一次遍历就可以找到最接近的4/5/6，距离和id记录到这6个变量中
        int min_id1 = -1, min_id2 = -1, min_id3 = -1;
        if (workbenches[i].type == 9) {
            have_9 = 1; //有9用make_choice,防止图4炸了
        }
        if (workbenches[i].type == 7) {
            have_7 = 1; //有9用make_choice,防止图4炸了
        }
        if (workbenches[i].type == 7 && (compare_pos(workbenches[i].pos,workbench_7.begin()) || compare_pos(workbenches[i].pos ,workbench_7.end()-1))){   //如果当前平台是是7
            for (int j = 0;j < workbench_cnt;j++) {   //寻找最近的工作台
                /*if (workbenches[j].be_matched != 1) {  //当前工作台还未被匹配
                }*/
                double new_dis = my_distance(workbenches[i].pos, workbenches[j].pos);   //两工作台间的距离
                if (workbenches[j].type == 4 && new_dis < dis1) {    //判断是否为4且距离小于目前已遍历到的最近的4
                    dis1 = new_dis;
                    min_id1 = j;
                }
                else if (workbenches[j].type == 5 && new_dis < dis2) {   //判断是否为4且距离小于目前已遍历到的最近的5
                    dis2 = new_dis;
                    min_id2 = j;
                }
                else if (workbenches[j].type == 6 && new_dis < dis3) {  //判断是否为4且距离小于目前已遍历到的最近的5
                    dis3 = new_dis;
                    min_id3 = j;
                }
            }
            workbenches[min_id1].be_matched = 1;    //表示min_id1（此时为可生产4且距离最近的平台id）对应的平台为被该当前台（7）送原料4，之后同理
            workbenches[min_id2].be_matched = 1;
            workbenches[min_id3].be_matched = 1;
            workbenches[i].match.insert(pair<int, int>(4, min_id1)); //加入材料4的匹配项
            workbenches[i].match.insert(pair<int, int>(5, min_id2)); //加入材料5的匹配项
            workbenches[i].match.insert(pair<int, int>(6, min_id3)); //加入材料6的匹配项
        }
        if (workbenches[i].type == 6) { //如果当前平台是是6
            for (int j = 0;j < workbench_cnt;j++) {
                /*if (workbenches[j].be_matched != 1) {  //当前工作台还未被匹配
                }*/
                double new_dis = my_distance(workbenches[i].pos, workbenches[j].pos);
                if (workbenches[j].type == 2 && new_dis < dis1) {
                    dis1 = new_dis;
                    min_id1 = j;
                }
                else if (workbenches[j].type == 3 && new_dis < dis2) {
                    dis2 = new_dis;
                    min_id2 = j;
                }
            }
            workbenches[min_id1].be_matched = 1;  //这里注释掉是觉得1/2/3生产很快，可被多个工作台共享（调试获取的结论）
            workbenches[min_id2].be_matched = 1;
            workbenches[i].match.insert(pair<int, int>(2, min_id1));//加入材料2的匹配项
            workbenches[i].match.insert(pair<int, int>(3, min_id2));//加入材料3的匹配项
        }
        if (workbenches[i].type == 5) { //如果当前平台是是5
            for (int j = 0;j < workbench_cnt;j++) {
                double new_dis = my_distance(workbenches[i].pos, workbenches[j].pos);
                if (workbenches[j].type == 1 && new_dis < dis1) {
                    dis1 = new_dis;
                    min_id1 = j;
                }
                else if (workbenches[j].type == 3 && new_dis < dis2) {
                    dis2 = new_dis;
                    min_id2 = j;
                }
            }
            workbenches[min_id1].be_matched = 1;  //这里注释掉是觉得1/2/3生产很快，可被多个工作台共享（调试获取的结论）
            workbenches[min_id2].be_matched = 1;
            workbenches[i].match.insert(pair<int, int>(1, min_id1));//加入材料1的匹配项
            workbenches[i].match.insert(pair<int, int>(3, min_id2));//加入材料3的匹配项
        }
        if (workbenches[i].type == 4) { //如果当前平台是是4
            for (int j = 0;j < workbench_cnt;j++) {
                double new_dis = my_distance(workbenches[i].pos, workbenches[j].pos);
                if (workbenches[j].type == 1 && new_dis < dis1) {
                    dis1 = new_dis;
                    min_id1 = j;
                }
                else if (workbenches[j].type == 2 && new_dis < dis2) {
                    dis2 = new_dis;
                    min_id2 = j;
                }
            }
            workbenches[min_id1].be_matched = 1;  //这里注释掉是觉得1/2/3生产很快，可被多个工作台共享（调试获取的结论）
            workbenches[min_id2].be_matched = 1;
            workbenches[i].match.insert(pair<int, int>(1, min_id1));//加入材料1的匹配项
            workbenches[i].match.insert(pair<int, int>(2, min_id2));//加入材料2的匹配项
        }
    }
}

bool find_workbench(int workbenchId, int& go_workbenchId, int materialId) {
    double dis = 2501.0;
    int closet_worktable_id = -1;
    double closet_dis = 2501.0;
    for (int i = 0;i < workbench_cnt; i++) {
        if (workbenches[i].type == materialId) {   //该工作台生产的产品为我要的原材料
            double new_dis = my_distance(workbenches[i].pos, workbenches[workbenchId].pos);
            if (workbenches[i].product_bit == 1 && workbenches[i].product_lock == 0) {  //该工作台有产品且未被锁,即可以取它的产品
                if (new_dis < dis) {    //找最近的
                    go_workbenchId = i;
                    dis = new_dis;
                }
            }
        }
    }
    if (go_workbenchId != -1)  //找到了可以去取产品的平台
        return true;
    else {
        //go_workbenchId = closet_worktable_id;   //没找到能取的，但是找到了能生产我要的原材料的最近的工作台
        return false;
    }
}




int main() {


    // 记录程序开始运行的时间点
    init();
    readmap();
    while (scanf("%d", &frameID) != EOF)
    {
        read_frame_info(money);
        printf("%d\n", frameID);

        if (frameID == 1) {   //第一帧，作出决策
            plan_strategy();
        }
        fflush(stdout);

        collide_id = avoidCollide();//判断哪些机器人要避免碰撞
        map<int, int> make_choice_second_id;
        for (int i = 0;i < workbench_cnt;i++) {
            if (workbenches[i].type == 7  && frameID < 8399) {
                if(compare_pos(workbenches[i].pos,workbench_7.begin()) || compare_pos(workbenches[i].pos ,workbench_7.end()-1)){
                    if(compare_pos(workbenches[i].pos ,workbench_7.end()-1))
                        fprintf(stderr,"end7\n");
                    if (workbenches[i].remaining_time > 0) {
                        make_choice_second_id[i] = 0;
                    }
                    else {
                        make_choice_second_id[i] = 1;
                    }
                }
            }
        }

        for (int i = 0;i < workbench_cnt;i++)
        {
            if (workbenches[i].type == 9)
                workbench_9_id = i;
        }
        for (map<int, int>::iterator it = make_choice_second_id.begin();it != make_choice_second_id.end();it++) {
            if (it->second == 1) {
                make_choice_sec(it->first);
            }
        }
        for (map<int, int>::iterator it = make_choice_second_id.begin();it != make_choice_second_id.end();it++) {
            if (it->second == 0) {
                make_choice_sec(it->first);
            }
        }
        for (int robotId = 0; robotId < 4; robotId++) {
            if (collide_id & (1 << robotId))//如果这个机器人已经进行过碰撞避免，就不在进行后续计算
                continue;
            if ((robots[robotId].sell == 0) && (robots[robotId].buy == 0))
            {
                if (have_9) {
                    if (make_choice(robotId))
                    {
                        robots[robotId].buy = 1;
                        robots[robotId].sell = 1;
                    }
                }
                else {
                    for (int i = 0;i < workbench_cnt;i++) {
                        if (workbenches[i].be_matched == 1 && tp_worktable[workbenches[i].type].raw_material != 0) {
                            //fprintf(stderr,"free robot\n");
                            int a = workbenches[i].material_bits;
                            int b = tp_worktable[workbenches[i].type].raw_material;
                            int go_workbenchId = -1;
                            for (int bit = 1;bit < 7;bit++) {
                                if ((a & (1 << bit)) != (b & (1 << bit))) {
                                    if (check_lock(0, bit, 0, i) == 0 && find_workbench(i, go_workbenchId, bit)) {
                                        fprintf(stderr, "free robot\n");
                                        break;
                                    }
                                }
                            }
                            if (go_workbenchId != -1) {
                                lock(0, workbenches[go_workbenchId].type, robotId, i);
                                lock(1, workbenches[go_workbenchId].type, robotId, go_workbenchId);
                                robots[robotId].workbench_to_buy_id = go_workbenchId;
                                robots[robotId].workbench_to_sell_id = i;
                                robots[robotId].buy = 1;
                                robots[robotId].sell = 1;
                                //fprintf(stderr,"free robot\n");
                                break;
                            }
                        }
                    }
                    if ((robots[robotId].sell == 0) && (robots[robotId].buy == 0)) {
                        printf("rotate %d 0\n", robotId);
                        printf("forward %d 1\n", robotId);
                    }
                }
                /*if (make_choice(robotId))
                {
                    robots[robotId].buy = 1;
                    robots[robotId].sell = 1;
                }*/
                /*if(have_9){
                }else{
                    printf("rotate %d 0\n",robotId);
                    printf("forward %d 1\n", robotId);
                }*/
            }

            bool collision_risk = is_collision_risk(robotId);
            if (!(collide_id & (1 << robotId)))
                give_command(robotId, collision_risk);
        }
        printf("OK\n");
        fflush(stdout);
    }
    return 0;
}