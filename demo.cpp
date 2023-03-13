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

using namespace std;

struct type_worktable_struct//每种类型的工作台需要原材料raw_material，需要周期period，生产出了produce
{
    int raw_material;
    int period;
    int produce;
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
type_worktable_struct tp_worktable[10];
Workbench workbenches[55];//为了通过工作台ID访问工作台
Robot robots[4];//为了通过机器人ID访问机器人
map<int,int> full;//哪些工作台有产品等待取走
map<int,int> waiting_material[8];//第i号材料被第几个id的工作台需要
int buy, sell;//是否进行购买

//初始化工作在等待的材料
void init_material_to_bench(int type, int id)
{
    if (workbenches[id].product_bit == 0)
    {
        for (int i = 1;i <= 7;i++)
        {
            if ((1 << i) & tp_worktable[type].raw_material && ((1 << i) & workbenches[i].raw_bits) == 0 )
            {
                waiting_material[i][id] = 1;//这个工作台正在等待这种原材料
            }
        }
    }
    else
    {
        full[id] = 1;
    }
}

// 读取一帧的信息
void read_frame_info(int& money) {
    cin >> money >> workbench_cnt;
    for (int i = 0; i < workbench_cnt; i++) {
        Workbench& workbench = workbenches[i];
        cin >> workbench.type;
        workbench.pos = read_point();
        cin >> workbench.remaining_time >> workbench.raw_bits >> workbench.product_bit;
        init_material_to_bench(workbench.type, i);
    }
    for (int i = 0; i < 4; i++) {
        Robot& robot = robots[i];
        cin >> robot.workbench_id >> robot.carrying_type >> robot.time_value_coef >> robot.collision_value_coef;
        // 读入机器人的携带物品的时间价值系数和碰撞价值系数
        // 读入机器人的角速度、线速度、朝向和位置
        cin >> robot.angular_speed >> robot.linear_speed.x >> robot.linear_speed.y
            >> robot.facing_direction >> robot.pos.x >> robot.pos.y;
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
    tp_worktable[4].raw_material = (1 << 1) & (1 << 2);
    tp_worktable[4].produce = 4;

    tp_worktable[5].period = 500;
    tp_worktable[5].raw_material = (1 << 1) & (1 << 3);
    tp_worktable[5].produce = 5;

    tp_worktable[6].period = 500;
    tp_worktable[6].raw_material = (1 << 2) & (1 << 3);
    tp_worktable[6].produce = 6;

    tp_worktable[7].period = 1000;
    tp_worktable[7].raw_material = (1 << 4) & (1 << 5) & (1 << 6);
    tp_worktable[7].produce = 7;

    tp_worktable[8].period = 1;
    tp_worktable[8].raw_material = (1 << 7);
    tp_worktable[8].produce = 0;

    tp_worktable[9].period = 1;
    tp_worktable[9].raw_material = (1 << 1) & (1 << 2) & (1 << 3) & (1 << 4) & (1 << 5) & (1 << 6) & (1 << 7);
    tp_worktable[9].produce = 0;

}

//载入地图
void readmap() {
    for (int i = 1;i <= 100;i++)
    {
        for (int j = 1;j <= 100;j++)
        {
            cin >> mp[i][j];
        }
    }
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
    int min_id = 0;
    min_dis = 0x3ffff;
    if (robots[robotid].carrying_type == 0)
    {
        for (map<int,int>::iterator itbegin = full.begin();itbegin != full.end();itbegin++)//遍历所有东西的
        {
            if (itbegin->second == 0)
            {
                continue;
            }
            double dis = my_distance(robots[robotid].pos, workbenches[itbegin->first].pos);
            if (min_dis > dis)
            {
                min_dis = dis;
                min_id = itbegin->first;
            }
        }
        buy = 1;//这个机器人要买东西了
    }
    else//遍历自己要找到的
    {
        for (map<int,int>::iterator itbegin = waiting_material[robots[robotid].carrying_type].begin();itbegin != waiting_material[robots[robotid].carrying_type].end();itbegin++)
        {
            if (itbegin->second==0)
            {
                continue;
            }
            double dis = my_distance(robots[robotid].pos, workbenches[itbegin->first].pos);
            if (min_dis > dis)
            {
                min_dis = dis;
                min_id = itbegin->first;
            }
        }
        sell = 1;//这个机器人要卖东西了
    }
    return min_id;
}

double cal_angle(int table_id, int robot_id){
    double dx = workbenches[table_id].pos.x - robots[robot_id].pos.x;
    double dy = workbenches[table_id].pos.y - robots[robot_id].pos.y;
    double angle = atan2(dy, dx) - robots[robot_id].facing_direction;
    if (angle > pi) angle -= 2 * pi;
    if (angle < -pi) angle += 2 * pi;
    return angle;
}

void update_robot(int robotId, int table_id)
{
    if (buy)
    {
        robots[robotId].carrying_type = tp_worktable[workbenches[table_id].type].produce;
        workbenches[table_id].product_bit = 0;
        workbenches[table_id].remaining_time = 0;
        full[table_id] = 0;
    }
    if (sell)
    {
        workbenches[table_id].raw_bits &= 1 << robots[robotId].carrying_type;
        robots[robotId].carrying_type = 0;
        waiting_material[robots[robotId].carrying_type][table_id] = 0;
    }
}

int main() {
    init();
    readmap();
    int frameID, money;
    vector<Workbench> workbenches;
    vector<Robot> robots;
    while (scanf("%d", &frameID) != EOF) {
        read_frame_info(money);
        printf("%d\n", frameID);
        fflush(stdout);
        //update_workbench();
        double lineSpeed = 3;
        double angleSpeed = 1.5;
        double my_distance;
        for (int robotId = 0; robotId < 4; robotId++) {
            buy = 0, sell = 0;//清空上一轮的购买标记
            int table_id = best_fit(my_distance, robotId);
            lineSpeed = my_distance / (1.0 / 50);//一帧走过去
            int flag = 1;
            if (lineSpeed > 6.0)
            {
                flag = 0;
                lineSpeed = 6.0;
            }
            angleSpeed = cal_angle(table_id, robotId) / (1.0 / 50);//一帧转完
            printf("rotate %d %f\n", robotId, angleSpeed);
            fflush(stdout);
            printf("forward %d %f\n", robotId, lineSpeed);
            fflush(stdout);
            if (flag && buy)
            {
                update_robot(robotId, table_id);
                printf("buy %d\n", robotId);
                fflush(stdout);
            }
            if (flag && sell)
            {
                update_robot(robotId, table_id);
                printf("sell %d\n", robotId);
                fflush(stdout);
            }
        }
        printf("OK\n");
        fflush(stdout);
    }
    return 0;
}
