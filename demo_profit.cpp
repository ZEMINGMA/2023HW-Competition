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
    int raw_material;//ÿ�����͵Ĺ���̨��Ҫԭ����raw_material
    int period;//��Ҫ����period
    int produce;//��������produce
        };
struct Point
        {
    double x, y;
        };

// ����̨
struct Workbench {
    int type; // ����̨����
    Point pos; // ����
    int remaining_time; // ʣ������ʱ�䣨֡����
    int raw_bits; // ԭ���ϸ�״̬��������λ������������ 48��110000����ʾӵ����Ʒ 4 �� 5
    int product_bit; // ��Ʒ��״̬��0 ��ʾ�ޣ�1 ��ʾ��
    int product_lock;//��ǰ��Ʒ�Ƿ��л����˱���
    int raw_lock;//��ǰ���ϸ��Ƿ��л����˱���
};

// ������
struct Robot {
    int workbench_id; // ��������̨ ID��-1 ��ʾ��ǰû�д����κι���̨������[0,����̨����-1]��ʾĳ����̨���±꣬��0��ʼ��������˳�򶨡���ǰ�����˵����й��򡢳�����Ϊ����Ըù���̨����
    int carrying_type; // Я����Ʒ���ͣ�0 ��ʾδЯ����Ʒ��1-7 ��ʾ��Ӧ��Ʒ
    double time_value_coef; // ʱ���ֵϵ����Я����ƷʱΪ [0.8,1] �ĸ���������Я����ƷʱΪ 0
    double collision_value_coef; // ��ײ��ֵϵ����Я����ƷʱΪ [0.8,1] �ĸ���������Я����ƷʱΪ 0
    double angular_speed; // ���ٶȣ���λΪ����/�룬������ʾ��ʱ�룬������ʾ˳ʱ��
    Point linear_speed; // ���ٶȣ��ɶ�ά�����������ٶȣ���λΪ��/�룬ÿ����50֡
    double facing_direction; // ���򣬱�ʾ�����˵ĳ��򣬷�ΧΪ [-��,��]������ʾ����0 ��ʾ�ҷ��򣬦�/2 ��ʾ�Ϸ���-��/2 ��ʾ�·���
    Point pos; // ����
    Point before_pos;
    int buy;//׼������
    int sell;//׼������
    int destroy;//
    int table_id;//ǰ���Ĺ���̨ID
};

// ��ȡһ�� Point ���͵ı���
Point read_point() {
    Point point;
    cin >> point.x >> point.y;
    return point;
};

//ȫ�ֱ������岿��
double const pi = 3.1415926535;
char mp[105][105];
int workbench_cnt; // ���Ϲ���̨������
int frameID, money;
type_worktable_struct tp_worktable[10];
Workbench workbenches[55];//Ϊ��ͨ������̨ID���ʹ���̨
Robot robots[4];//Ϊ��ͨ��������ID���ʻ�����

//��һ��������0��ʾԭ���ϣ�1��ʾ��Ʒ���ڶ����ǲ�Ʒ��ԭ����id
void lock(int raw_or_product, int raw_or_product_id, int robotid, int workbenchid)//����
{
    if(workbenches[workbenchid].type==8 || workbenches[workbenchid].type==9 )
        return ;
    if (raw_or_product == 0)//�����ԭ����
        {
        fprintf(stderr, "before lock raw_lock:%d,workbenchid:%d\n", workbenches[workbenchid].raw_lock, workbenchid);
        workbenches[workbenchid].raw_lock |= 1 << raw_or_product_id;//��������λ���㣬��Ӧ���ϸ���1
        fprintf(stderr, "after lock raw_lock:%d,workbenchid:%d\n", workbenches[workbenchid].raw_lock,workbenchid);

        //Sleep(100);
        }
    else//����ǲ�Ʒ
    {
        workbenches[workbenchid].product_lock = robotid + 1;//��Ʒ���û����˱��
    }
}

//��һ��������0��ʾԭ���ϣ�1��ʾ��Ʒ���ڶ����ǲ�Ʒ��ԭ����id
void unlock(int raw_or_product, int raw_or_product_id, int robotid, int workbenchid)
{
    if(workbenches[workbenchid].type==8 || workbenches[workbenchid].type==9 )
        return ;
    if (raw_or_product == 0)//�����ԭ����
        {
        fprintf(stderr, "before unlock raw_lock:%d,workbenchid:%d\n", workbenches[workbenchid].raw_lock, workbenchid);
        workbenches[workbenchid].raw_lock &= ~(1 << raw_or_product_id);//�����������Ӧ���ϸ���1
        fprintf(stderr, "after unlock raw_lock:%d,workbenchid:%d\n", workbenches[workbenchid].raw_lock, workbenchid);
        workbenches[workbenchid].raw_bits |= 1 << robots[robotid].carrying_type;//�������sell��ʱ�򣬷�ֹmainѭ������һ������������һ֡û�еõ����ϸ��Ѿ���װ������Ϣ
        //Sleep(100);
        }
    else//����ǲ�Ʒ
    {
        workbenches[workbenchid].product_lock = 0;//��ղ�Ʒ��
        workbenches[workbenchid].product_bit = 0;//�������buy��ʱ�򣬷�ֹmainѭ������һ������������һ֡û�еõ����ϸ��Ѿ���װ������Ϣ
    }
}

//��һ��������0��ʾԭ���ϣ�1��ʾ��Ʒ���ڶ����ǲ�Ʒ��ԭ����id
int check_lock(int raw_or_product, int raw_or_product_id, int robotid, int workbenchid)//1��ʾ�����ˣ�0��ʾû�б���
{
    if (raw_or_product == 0)//�����ԭ����
        {
        if ((workbenches[workbenchid].raw_lock & (1 << raw_or_product_id)) != 0)//�����������ӦλΪ1
            {
            return 1;//����������������
            }
        }
    else//����ǲ�Ʒ
    {
        if (workbenches[workbenchid].product_lock != 0 && workbenches[workbenchid].product_lock != robotid + 1)//�����Ʒ�����������Ҳ����Լ�����
            {
            return 1;//����������������
            }
    }
    return 0;//û�б���
}

// ���ɷ�ΧΪ[min, max]�����������
double random_double(double min, double max) {
    static std::mt19937 generator(std::time(0));
    std::uniform_real_distribution<double> distribution(min, max);
    return distribution(generator);
}
// ��ȡһ֡����Ϣ
void read_frame_info(int& money) {
    cin >> money >> workbench_cnt;
    for (int i = 0; i < workbench_cnt; i++) {
        cin >> workbenches[i].type;
        workbenches[i].pos = read_point();
        cin >> workbenches[i].remaining_time >> workbenches[i].raw_bits >> workbenches[i].product_bit;
    }
    for (int i = 0; i < 4; i++) {
        cin >> robots[i].workbench_id >> robots[i].carrying_type >> robots[i].time_value_coef >> robots[i].collision_value_coef;
        // ��������˵�Я����Ʒ��ʱ���ֵϵ������ײ��ֵϵ��
        // ��������˵Ľ��ٶȡ����ٶȡ������λ��

        if (frameID % 300 == 0) {
            robots[i].before_pos.x = robots[i].pos.x;
            robots[i].before_pos.y = robots[i].pos.y;
        }

        cin >> robots[i].angular_speed >> robots[i].linear_speed.x >> robots[i].linear_speed.y >> robots[i].facing_direction >> robots[i].pos.x >> robots[i].pos.y;
    }
    // ����һ���ַ������ж��Ƿ��������
    string ok;
    cin >> ok;
    assert(ok == "OK"); // ���һ�б�����OK
}

void init()//��ʼ��ÿ�����͵Ĺ���̨����Ϣ
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

//�����ͼ
void readmap() {
    for (int i = 1;i <= 100;++i)
        for (int j = 1;j <= 100;++j)
            cin >> mp[i][j];
        string ok;
        cin >> ok;
        cout << ok << endl;
        fflush(stdout);
        assert(ok == "OK"); // ���һ�б�����OK
}

double my_distance(Point p1, Point p2)//����������֮�������
{
    return sqrt((p1.x - p2.x) * (p1.x - p2.x) + (p1.y - p2.y) * (p1.y - p2.y));
}

int best_fit(int robotId) {
    double max_profit = -1;
    int best_workbench_id = -1;

    for (int i = 0; i < workbench_cnt; i++) {
        // ������ǰ����̨�����������
        if (workbenches[i].product_lock != 0) {
            continue;
        }

        // ��������
        int product_price = tp_worktable[workbenches[i].type].produce * 100; // ����ÿ����Ʒ�ļ۸������ŵ� 100 ��
        double distance_to_workbench = my_distance(robots[robotId].pos, workbenches[i].pos);
        double time_to_reach_workbench = distance_to_workbench / 5.0; // ������������ٶ�Ϊ 3.0 ��/��
        double time_to_return_and_sell = time_to_reach_workbench; // ���践�غͳ��۲�Ʒ����ʱ���뵽�﹤��̨��ͬ
        double total_time = time_to_reach_workbench + time_to_return_and_sell;
        double profit = product_price / total_time;

        // ��������������ѹ���̨
        if (profit > max_profit) {
            max_profit = profit;
            best_workbench_id = i;
        }
    }

    return best_workbench_id;
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
        double distance=1.0;


        for (int robotId = 0; robotId < 4; robotId++) {

            if ((robots[robotId].sell == 0) && (robots[robotId].buy == 0)) {
                robots[robotId].table_id = best_fit(robotId);  //ֻ�е���������Ҫ���й�����߳��۵�ʱ����ȥfit
                //fprintf(stderr, "robotId=%d go to table=%d with type %d buy=%d  sell=%d\n", robotId, robots[robotId].table_id, workbenches[robots[robotId].table_id].type, robots[robotId].buy, robots[robotId].sell);
                if (robots[robotId].table_id == -1 && robots[robotId].carrying_type != 0 && frameID % 5000 == 0) {
                    printf("destroy %d\n", robotId);
                    //fprintf(stderr, "robots %d destroy WITH ERROR\n", robotId);
                }

            }
            //sleep(1);


            if (abs(robots[robotId].before_pos.x - robots[robotId].pos.x) < 0.005 && abs(robots[robotId].before_pos.y - robots[robotId].pos.y) < 0.005)
            {
                //robots[robotId].destroy=1;
            }

            if (robots[robotId].table_id != -1)
            {
                //workbenches[robots[robotId].table_id].lock = robotId+1;//��
                double move_distance = distance / (1.0 / 50);//���ٶ�
                double rotate_angle = cal_angle(robots[robotId].table_id, robotId) / (1.0 / 50);//���ٶ�
                if (move_distance > 6.0) {
                    move_distance = 6.0;
                }
                if (move_distance < -2.0) {
                    move_distance = -2.0;
                }
                if (rotate_angle > 3.14159265358979323846) {//M_PI�����ݣ����ּ���
                    rotate_angle = 3.14159265358979323846;
                }
                if (rotate_angle < -1 * 3.14159265358979323846) {
                    rotate_angle = -1 * 3.14159265358979323846;
                }
                printf("rotate %d %f\n", robotId, rotate_angle);
                fflush(stdout);
                double random_distance=4+random_double(-0.8,0.8);
                if (abs(rotate_angle) > 3)
                    printf("forward %d %f\n", robotId,random_distance);//����������޸�תȦȦ�Ĵ�С�����ܿ��Գ�ȥ
                    else if (my_distance(robots[robotId].pos, workbenches[robots[robotId].table_id].pos) < 1)
                        printf("forward %d 2\n", robotId);
                    else
                        printf("forward %d %f\n", robotId, move_distance);
                    //fprintf(stderr,"forward %d %f rotate %f\n", robotId, move_distance,rotate_angle);
                    fflush(stdout);

                    if (robots[robotId].workbench_id == robots[robotId].table_id && robots[robotId].buy == 1) {
                        printf("buy %d\n", robotId);
                        robots[robotId].buy = 0;
                        unlock(1, tp_worktable[workbenches[robots[robotId].table_id].type].produce, robotId, robots[robotId].table_id);//���Ʒ��
                        //workbenches[robots[robotId].table_id].lock =0;//����
                        fflush(stdout);
                    }
                    else if (robots[robotId].destroy == 1 && robots[robotId].sell == 1) {
                        printf("destroy %d\n", robotId);
                        //fprintf(stderr, "robots %d destroy\n", robotId);
                        robots[robotId].sell = 0;
                        robots[robotId].destroy = 0;
                        //workbenches[robots[robotId].table_id].lock = 0;//����
                        unlock(0, robots[robotId].carrying_type, robotId, robots[robotId].table_id);//�������
                        fflush(stdout);
                    }
                    else if (robots[robotId].workbench_id == robots[robotId].table_id && robots[robotId].sell == 1) {
                        printf("sell %d\n", robotId);
                        robots[robotId].sell = 0;
                        //workbenches[robots[robotId].table_id].lock = 0;//����
                        unlock(0, robots[robotId].carrying_type, robotId, robots[robotId].table_id);//�������
                        fflush(stdout);
                    }
            }
        }
        printf("OK\n");
        fflush(stdout);
    }
    return 0;
}