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
    int workbench_to_buy_id ;
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
            //fprintf(stderr, "before lock material_lock:%d,workbenchid:%d\n", workbenches[workbenchid].material_lock, workbenchid);
            workbenches[workbenchid].material_lock |= 1 << material_or_product_id;//材料锁按位运算，相应材料格置1
            //fprintf(stderr, "after lock material_lock:%d,workbenchid:%d\n", workbenches[workbenchid].material_lock, workbenchid);
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
        if(robotid!=-1)
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

double cal_angle(int table_id, int robot_id) {
    double dx = workbenches[table_id].pos.x - robots[robot_id].pos.x;
    double dy = workbenches[table_id].pos.y - robots[robot_id].pos.y;
    double angle = atan2(dy, dx) - robots[robot_id].facing_direction;
    if (angle > pi) angle -= 2 * pi;
    if (angle < -1*pi) angle += 2 * pi;
    return angle;
}

double cal_time_value_coef(double holding_time,double maxX,double minRate )//持有时间 最大帧数 最小比例
{
    holding_time = holding_time *50;
    if (holding_time >= maxX)
        {return minRate;}
    double a1 = 1 - sqrt(1 - (1 - holding_time / maxX) * (1 - holding_time / maxX));
    return a1 * (1 - minRate) + minRate;
}

bool make_choice(int robotid) {
    double max_profit = -1;
    bool yes_or_no=false;
        robots[robotid].workbench_to_buy_id=-1;
        robots[robotid].workbench_to_sell_id=-1;
        for (int i = 0;i < workbench_cnt;++i)//遍历所有工作台(只要曾经有过成品就会出现在full里)
        {
            // 计算利润
            int product_type = workbenches[i].type;
            int product_price;
            int purchase_price;
            switch (product_type) {
                case 1:purchase_price=3000; product_price = 6000; break;
                case 2:purchase_price=4400; product_price = 7600; break;
                case 3:purchase_price=5800; product_price = 9200; break;
                case 4:purchase_price=15400; product_price = 22500; break;
                case 5:purchase_price=17200; product_price = 25000; break;
                case 6:purchase_price=19200; product_price = 27500; break;
                case 7:purchase_price=76000; product_price = 105000; break;
                default:purchase_price=0; product_price = 0; break;
            }

            if (workbenches[i].product_bit == 0)//如果没有生产出产品
                {continue;}

            if (check_lock(1, tp_worktable[workbenches[i].type].produce, robotid, i))//如果被锁了continue
                {continue;}
            
            int need = 0;//这里把need赋值为0
            for (int j = 0;j < workbench_cnt;++j)//遍历所有工作台(只要曾经有过成品就会出现在full里)
                {
                //如果这种类型的工作台不需要这种类型的材料
                if ((tp_worktable[workbenches[j].type].raw_material & (1 << (tp_worktable[workbenches[i].type].produce))) == 0)
                    {continue;}
                //如果工作台已经有了这种原材料
                if ((workbenches[j].material_bits & (1 << (tp_worktable[workbenches[i].type].produce))) != 0)
                    {continue;}
                //如果工作台已经的这种原材料已经被死锁了
                if ((workbenches[j].material_lock & (1 << (tp_worktable[workbenches[i].type].produce))) != 0)
                    {continue;}

                need = 1;
                double to_buy_time = my_distance(robots[robotid].pos, workbenches[i].pos)/5.5;
                double to_sell_time= my_distance(workbenches[i].pos,workbenches[j].pos)/4.5;
                double total_time = to_buy_time + to_sell_time;
                product_price=product_price*cal_time_value_coef(to_sell_time,9000,0.8)-purchase_price;
                double profit = product_price / total_time ;
                    // 更新最大利润和最佳工作台
                    if (profit > max_profit) 
                    {
                        max_profit = profit;
                        robots[robotid].workbench_to_buy_id=i;
                        robots[robotid].workbench_to_sell_id=j;
                    }
                }
            if (need == 0) continue;//继续寻找下一个工作台，而不是break不找所有工作台
        }

        if(max_profit!=-1)
        {
                lock(1, tp_worktable[workbenches[robots[robotid].workbench_to_buy_id].type].produce, robotid, robots[robotid].workbench_to_buy_id);//机器人找到了生产出的产品，需要加产品锁
                lock(0, tp_worktable[workbenches[robots[robotid].workbench_to_buy_id].type].produce, robotid, robots[robotid].workbench_to_sell_id);//机器人找到了空闲的材料格，需要给这个材料格加材料锁
                yes_or_no=true;
        } 
        return yes_or_no;
}

bool is_collision_risk(int robot_id) {
    bool risk=false;
    double predicted_x = robots[robot_id].pos.x + robots[robot_id].linear_speed.x * time_step*25;
    double predicted_y = robots[robot_id].pos.y + robots[robot_id].linear_speed.y * time_step*25;
    risk=(predicted_x < 0.6 || predicted_x > map_width-0.6 || predicted_y < 0.6 || predicted_y > map_height-0.6);

    if (risk)
    {
       fprintf(stderr,"risk!!!!\n");
    }
      return risk;
}

double isCollide(double& my_rotate, Robot robot1, Robot robot2) {
    double robots_distance = my_distance(robot1.pos, robot2.pos);//计算两个机器人距离
    double crash_time = 50 * robots_distance / (my_distance(robot1.linear_speed, Point{ 0,0 }) + my_distance(robot2.linear_speed, Point{ 0,0 }));//计算直线距离还有多少帧
    double judge_crash_angle = pi / 4;//判断相撞的夹角范围
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
    if (int(crash_time) < 20 && sita3 <= judge_crash_angle)  //控制反应帧为20帧
    {
        if (-judge_crash_angle < sita && sita < 0 && 0 < sita2 && sita2 < judge_crash_angle)//下面分别对应6张图
        {
            if (-sita < sita2)
            {
                my_rotate = -1*pi;//顺时针转是负的
                return true;
            }
            else
            {
                my_rotate = pi;//逆时针转是正的
                return true;
            }
        }
        else if (0 < sita && sita < judge_crash_angle && -judge_crash_angle < sita2 && sita2 < 0)
        {
            if (sita < -sita2)
            {
                my_rotate = pi;
                return true;
            }
            else
            {
                my_rotate = -1*pi;
                return true;
            }
        }
        else if (-judge_crash_angle < sita && sita < 0 && -judge_crash_angle < sita2 && sita2 < 0)
        {
            my_rotate = pi;
            return true;
        }
        else if (0 < sita && sita < judge_crash_angle && 0 < sita2 && sita2 < judge_crash_angle)
        {
            my_rotate = -1*pi;
            return true;
        }
        else if (sita == 0)//如果对撞，随便左右转
        {
            my_rotate = pi;
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
                printf("forward %d 2\n", j);//减速，避免碰撞转不过来
                fflush(stdout);
                printf("forward %d 2\n", i);//减速，避免碰撞转不过来
                fflush(stdout);
                collide_id |= 1 << j;//记录哪些机器人会在这一帧进行计算
            }
        }
    }
    return collide_id;//返回机器人编号
}

double clamp(double value, double min_value, double max_value) {
    return std::max(min_value, std::min(value, max_value));
}

void give_command(int robotId,bool collision_risk){
        double lineSpeed = 3;
        double angleSpeed = 1.5;
        double distance = 1.0;
        double random_distance = 2 + random_double(-0.8, 0.4);

            if(robots[robotId].buy==1 )
            {
                double lineSpeed = distance / time_step;//线速度
                double angleSpeed = cal_angle(robots[robotId].workbench_to_buy_id, robotId) / time_step;//加速度

                lineSpeed=clamp(lineSpeed,-2,6);
                angleSpeed=clamp(angleSpeed,-1 * pi,pi);

                printf("rotate %d %f\n", robotId, angleSpeed);
                fflush(stdout);
                
                if (abs(angleSpeed) > 3 && collision_risk)
                    printf("forward %d 0.5\n", robotId);
                else if (collision_risk)
                        printf("forward %d 4\n", robotId);
                else
                        printf("forward %d %f\n", robotId, lineSpeed);   
                fflush(stdout);

                if (robots[robotId].workbench_id == robots[robotId].workbench_to_buy_id && robots[robotId].buy == 1) {
                        printf("buy %d\n", robotId);
                        robots[robotId].buy = 0;
                        unlock(1, tp_worktable[workbenches[robots[robotId].workbench_to_buy_id].type].produce, robotId, robots[robotId].workbench_to_buy_id);//解产品锁
                        fflush(stdout);
                    }
            }

            else if(robots[robotId].sell==1 )
            {
                double lineSpeed = distance / time_step;//线速度
                double angleSpeed = cal_angle(robots[robotId].workbench_to_sell_id, robotId) / time_step;//加速度

                lineSpeed=clamp(lineSpeed,-2,6);
                angleSpeed=clamp(angleSpeed,-1 * pi,pi);

                printf("rotate %d %f\n", robotId, angleSpeed);
                fflush(stdout);

                if (abs(angleSpeed) > 3 && collision_risk)
                    printf("forward %d 0.5\n", robotId);
                else if (collision_risk)
                        printf("forward %d 4\n", robotId);
                else
                        printf("forward %d %f\n", robotId, lineSpeed);   
                fflush(stdout);

                if (robots[robotId].workbench_id == robots[robotId].workbench_to_sell_id && robots[robotId].sell == 1) {
                        printf("sell %d\n", robotId);
                        robots[robotId].sell = 0;
                        workbenches[robots[robotId].workbench_to_sell_id].material_count++;  //原材料增加

                        //std::thread([robotId]() {
                          //  std::this_thread::sleep_for(std::chrono::milliseconds(1000));
                        unlock(0, robots[robotId].carrying_type, robotId, robots[robotId].workbench_to_sell_id); // 解材料锁
                        //}).detach();
                        fflush(stdout);
                    }
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

        int collide_id = avoidCollide();//判断哪些机器人要避免碰撞
        for (int robotId = 0; robotId < 4; robotId++) {
            if (collide_id & (1 << robotId))//如果这个机器人已经进行过碰撞避免，就不在进行后续计算
                continue;
            if ((robots[robotId].sell == 0) && (robots[robotId].buy == 0)) 
            {
                    if(make_choice(robotId))
                    {
                        robots[robotId].buy=1;
                        robots[robotId].sell=1;
                    }
            }

            bool collision_risk = is_collision_risk(robotId);
            give_command(robotId,collision_risk);
        }
        printf("OK\n");
        fflush(stdout);
    }
    return 0;
}