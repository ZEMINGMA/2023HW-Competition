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
    tp_worktable[9].raw_material = (1 << 4) | (1 << 5) | (1 << 6) | (1 << 7);
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
        //fprintf(stderr, "before lock material_lock:%d,workbenchid:%d\n", workbenches[workbenchid].material_lock, workbenchid);
        workbenches[workbenchid].material_lock |= 1 << material_or_product_id;//材料锁按位运算，相应材料格置1
        //fprintf(stderr, "after lock material_lock:%d,workbenchid:%d\n", workbenches[workbenchid].material_lock, workbenchid);
    }
    else//如果是产品
    {
        if (material_or_product_id==1||material_or_product_id==3||material_or_product_id==2)
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
        fprintf(stderr, "before unlock material_lock:%d,workbenchid:%d\n", workbenches[workbenchid].material_lock, workbenchid);
        workbenches[workbenchid].material_lock &= ~(1 << material_or_product_id);//解材料锁，相应材料格置0
        fprintf(stderr, "after unlock material_lock:%d,workbenchid:%d\n", workbenches[workbenchid].material_lock, workbenchid);
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

        if(robotid==0)
            if(workbenches[i].type!=4){continue;}
        if(robotid==1)
            if(workbenches[i].type!=5){continue;}
        if(robotid==2)
            if(workbenches[i].type!=6){continue;}
        if(robotid==3)
            if(workbenches[i].type==1||workbenches[i].type==2||workbenches[i].type==3){continue;}

        if (workbenches[i].product_bit == 0)//如果没有生产出产品
        {
            continue;
        }

        if(workbenches[i].type==7){
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
            need = 1;
            double to_buy_time = my_distance(robots[robotid].pos, workbenches[i].pos) ;
            double to_sell_time = my_distance(workbenches[i].pos, workbenches[j].pos) ;
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

        if(robotid==0)
            if(workbenches[i].type==3){continue;}
        if(robotid==1)
            if(workbenches[i].type==2){continue;}
        if(robotid==2)
            if(workbenches[i].type==1){continue;}
        if(robotid==3)
            if(workbenches[i].type==start%4){continue;}
        if (workbenches[i].product_bit == 0)//如果没有生产出产品
        {
            continue;
        }

        if(workbenches[i].type==7){
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
            need = 1;
            double to_buy_time = my_distance(robots[robotid].pos, workbenches[i].pos) ;
            double to_sell_time = my_distance(workbenches[i].pos, workbenches[j].pos) ;
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
    Point force = {pos2.x - pos1.x, pos2.y - pos1.y};
    force.x *= coefficient;
    force.y *= coefficient;
    return force;
}

Point artificial_potential_field(int robotId) {
    // 设置参数
    double repulsive_coefficient = 30;  // 斥力系数
    double repulsive_distance = 4* 0.53;  // 斥力作用距离
    double attractive_coefficient = 100;  // 引力系数
    double robot_radius = 0.53;  // 机器人半径
    double exponential_factor = 30;  // 墙壁斥力增长的指数因子

    // 初始化合成势场
    Point synthetic_force = {0, 0};

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
            double repulsive_force = repulsive_coefficient * (1 / (distance - 2 * robot_radius) - 1 / repulsive_distance);
            double angle = angle_robot(robots[i].pos, robots[robotId].pos);  // 修改为从机器人i指向机器人robotId的角度

            // 计算斥力作用在机器人上的分量
            double repulsive_force_x = repulsive_force * cos(angle);
            double repulsive_force_y = repulsive_force * sin(angle);

            // 累加斥力
            synthetic_force.x += repulsive_force_x;
            synthetic_force.y += repulsive_force_y;
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
    } else if (diff < -pi) {
        diff += 2 * pi;
    }
    return diff;
}

void give_command(int robotId) {

    // 获取人工势场法结果
    Point apf_force = artificial_potential_field(robotId);
    double modified_angle = angle_robot(robots[robotId].pos, {robots[robotId].pos.x + apf_force.x, robots[robotId].pos.y + apf_force.y});

    // 计算角速度
    double angle_difference_to_target = angle_difference(robots[robotId].facing_direction, modified_angle);
    double angleSpeed = angle_difference_to_target / time_step;
    angleSpeed = clamp(angleSpeed, -max_angular_speed, max_angular_speed);

    // 根据购买和出售情况决定目标位置
    Point target_pos;
    if (robots[robotId].buy == 1) {
        target_pos = workbenches[robots[robotId].workbench_to_buy_id].pos;
    } else if (robots[robotId].sell == 1) {
        target_pos = workbenches[robots[robotId].workbench_to_sell_id].pos;
    }

    // 计算线速度
    double distance = my_distance(robots[robotId].pos, target_pos);
    double linearSpeed = distance / time_step;
    linearSpeed = clamp(linearSpeed, 0.0, max_linear_speed);

    printf("rotate %d %f\n", robotId, angleSpeed);
    fflush(stdout);


    printf("forward %d %f\n", robotId, linearSpeed);
    fflush(stdout);

    if (robots[robotId].workbench_id == robots[robotId].workbench_to_buy_id && robots[robotId].buy == 1) {
        printf("buy %d\n", robotId);
        robots[robotId].buy = 0;
        unlock(1, tp_worktable[workbenches[robots[robotId].workbench_to_buy_id].type].produce, robotId, robots[robotId].workbench_to_buy_id); // 解产品锁
        fflush(stdout);
    } else if (robots[robotId].workbench_id == robots[robotId].workbench_to_sell_id && robots[robotId].sell == 1) {
        printf("sell %d\n", robotId);
        robots[robotId].sell = 0;
        workbenches[robots[robotId].workbench_to_sell_id].material_count++; // 原材料增加
        unlock(0, robots[robotId].carrying_type, robotId, robots[robotId].workbench_to_sell_id); // 解材料锁

        if (workbenches[robots[robotId].workbench_to_sell_id].type == 7 && workbenches[robots[robotId].workbench_to_sell_id].product_bit == 1) {
            robots[robotId].workbench_to_buy_id = robots[robotId].workbench_to_sell_id;
            for (int j = 0; j < workbench_cnt; j++) {
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
            fflush(stdout);
        }
}





int main() {
    init();
    readmap();
    while (scanf("%d", &frameID) != EOF)
    {
        read_frame_info(money);
        printf("%d\n", frameID);
        fflush(stdout);

        for (int robotId = 0; robotId < 4; robotId++) {         
            if ((robots[robotId].sell == 0) && (robots[robotId].buy == 0))
            {
                if (make_choice(robotId))
                {
                    robots[robotId].buy = 1;
                    robots[robotId].sell = 1;
                }
            }
            give_command(robotId);
        }
        printf("OK\n");
        fflush(stdout);
    }
    return 0;
}