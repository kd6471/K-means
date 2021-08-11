#include <iostream>
#include <fstream>
#include <math.h>
#include <omp.h>
#include "DS_timer.h"
#include "DS_definitions.h"
#include "StdAfx.h"
#include "pGNUPlot.h"

using namespace std;

#define DIM 2
#define MIN_DIS INT_MAX
#define ERROR_RANGE 0.0001

int NUM_DATA = 1000000;
int k = 3;
int THREAD_NUM = 12;

int** readfile();
void writefile(string outtype, int* group, int** data_input);
int* kmeans(int k, int NUM_DATA, int** data_input);
int* kmeans_p(int k, int NUM_DATA, int** data_input);
void writeRandFile();


void main(int argc, char** argv) {
	
	if (argc > 1) {
		k = atoi(argv[1]);
	}

	if (argc > 2) {
		THREAD_NUM = atoi(argv[2]);
	}

	// CpGnuplot�� �����ڿ��� ���ڷ� wgnuplot.exe�� ��ü ��θ� �Ѱ��ش�.
	// Gnuplot�� ��ġ�� ��ο� ���� �� ���� �ٲ�� �Ѵ�.
	CpGnuplot plot("C:\\Program Files\\gnuplot\\bin\\wgnuplot.exe");

	DS_timer timer(2);

	printf("����ϴ� �ھ��� �� : %d\n", THREAD_NUM);
	timer.setTimerName(0, (char*)"[Serial]");
	timer.setTimerName(1, (char*)"[Parallel] Version 1");
	int** data_input = readfile();
	printf("�������� ���� : %d\n", NUM_DATA);
	writefile("Origin", NULL, data_input);

	int* group = new int[NUM_DATA];
	int* group_p = new int[NUM_DATA];

	timer.onTimer(0);
	group = kmeans(k,NUM_DATA,data_input);
	timer.offTimer(0);
	writefile("Serial", group, data_input);

	timer.onTimer(1);
	group_p = kmeans_p(k, NUM_DATA, data_input);
	timer.offTimer(1);
	writefile("Parallel", group_p, data_input);

	printf("[Serial] : %.5f ms ( * %0.5f)\n", timer.getTimer_ms(0), timer.getTimer_ms(0) / timer.getTimer_ms(0));
	printf("[Parallel] : %.5f ms ( * %0.5f)\n", timer.getTimer_ms(1), timer.getTimer_ms(1) / timer.getTimer_ms(0));
	
	plot.cmd("set term wxt 0"); //gnuplot 0�� ������� ���� ������ �ð�ȭ
	plot.cmd("set title \"Original data \"");
	plot.cmd("plot'C:\\temp\\out_data_origin.dat' notitle with points");

	plot.cmd("set term wxt 1"); //gnuplot 1�� ������� serial ó�� �� ������ �ð�ȭ
	plot.cmd("set title \"Serial \"");
	plot.cmd("set palette model RGB defined(0 'red', 1 'green',2 'blue')");
	plot.cmd("plot'C:\\temp\\out_data_serial.dat' u 1:2:($3) notitle with points palette");

	plot.cmd("set term wxt 2"); //gnu plot 2�� ������� Parallel ó�� �� ������ �ð�ȭ
	plot.cmd("set title \"Parallel \"");
	plot.cmd("set palette model RGB defined(0 'red', 1 'green',2 'blue')");
	plot.cmd("plot'C:\\temp\\out_data_parallel.dat' u 1:2:($3) notitle with points palette");

	getchar();
	return;
}


//////  file i/o  //////
int** readfile() {
	int d;
	
	ifstream indata;
	indata.open("data.txt");
	
	indata >> d;
	NUM_DATA = d;

	//int data_input[NUM_DATA][DIM]; //2���� �����͸� ����ϴϱ� [][2]�� ������
	int** data_input = new int* [NUM_DATA];
	for (int i = 0; i < NUM_DATA; i++) {
		*(data_input + i) = new int[DIM];
		for (int j = 0; j < DIM; j++) {
			indata >> d;
			data_input[i][j] = d;
		}
	}

	indata.close();
	return data_input;
}

void writefile(string outtype, int* group, int** data_input) {


	ofstream outdata;
	if (outtype == "Serial")
	{
		outdata.open("C:\\temp\\out_data_serial.dat");
		for (int i = 0; i < NUM_DATA; i++) {
			outdata << data_input[i][0] << ' '
				<< data_input[i][1] << ' '
				<< group[i] << endl;
		}
		outdata.close();
	}
	if (outtype == "Parallel")
	{
		outdata.open("C:\\temp\\out_data_parallel.dat");

		for (int i = 0; i < NUM_DATA; i++) {
			outdata << data_input[i][0] << ' '
			<< data_input[i][1] << ' '
			<< group[i] << endl;
		}
		outdata.close();
	}
	if (outtype == "Origin")
	{
		outdata.open("C:\\temp\\out_data_origin.dat");

			for (int i = 1; i < NUM_DATA; i++) {

				outdata << data_input[i][0] << ' '
					<< data_input[i][1] 
					 << endl;
			}
			outdata.close();
	}

}

void writeRandFile() {
	srand(time(NULL));
	ofstream outdata;
	outdata.open("C:\\temp\\data.txt");
	for (int i = 0; i < NUM_DATA; i++) {

		outdata << rand() % 20 + 5 << ' '
			<< rand() % 20 + 5 << endl;
		outdata << rand() % 30+60  << ' '
			<< rand() % 30+30 << endl;
		outdata << rand() % 30 + 20 << ' '
			<< rand() % 40 + 40 << endl;
	}

	outdata.close();
}

//////  k_means clustering  //////
int* kmeans(int k, int NUM_DATA, int** data_input) {
	int* group = new int[NUM_DATA];
	double ** means = new double*[k]; //K���� ����� ������ �迭 means�� �����

	for(int i = 0; i < k; i++)
		means[i] = new double[2]; //means�� K�� ��ŭ ���� �Ҵ�

	////  set new means  ////
	for(int i = 0; i < k; i++) 
		for(int j = 0; j < 2; j++)
			means[i][j] = data_input[i+4][j];  //means�� ������ ���(�ƹ� ������)�� ä���
	

	////  clustering  ////
	
	for (int s = 0; s < INT_MAX; s++) {
		int flag = 0;
		
		for(int i = 0; i < NUM_DATA; i++) {
			double min_dis = MIN_DIS;
			int min = -1;
			
			// calculating distances //
			for(int j = 0; j < k; j++) {
				double dis = 0;
				for(int l = 0; l < 2; l++)
					dis += pow(float(data_input[i][l] - means[j][l]),2); // means�� data ���� �Ÿ��� �缭 ���� ����� �Ÿ��� ����
				if(dis < min_dis) {
					min_dis = dis;
					min = j; //���� ����� means(���)�� �ε����� ������
				}
			}

			// set groups //
			group[i] = min; //���� ����� ��հ� ���� �׷쿡 ���� 
		}
	
		// allocate new memory for calculate //
		double **temp = new double*[k];
		int *count = new int[k];

		for(int i = 0; i < k; i++) {
			count[i] = 0;
			temp[i] = new double[2];
			for(int j = 0; j < 2; j++)
				temp[i][j] = 0;
		}

		// calculating new means // //���ο� ����� ���ϰ� ����� �ϳ��� �ٸ��ٸ� �ٽ� ���
		for(int i = 0; i < NUM_DATA; i++) {
			count[group[i]]++;
			for(int j = 0; j < 2; j++)
				temp[group[i]][j] += data_input[i][j];
		}

		for(int i = 0; i < k; i++)
			for(int j = 0; j < 2; j++) {
				temp[i][j] /= count[i]; //���� ����� ��հ�
				if(fabs(temp[i][j] - means[i][j]) > 0.0001) { //fabs()<-���� ó�� �Լ�
					flag++; //����� �ٸ� ��� flag++;
					means[i][j] = temp[i][j];
				}
			}

		// break point //
		if(flag == 0) //����� ���� �ߴµ� ����� �ٸ��� ���� ��� ���ѷ��� break
			break;
	}
	
	////  freeing memory  ////
	for(int i = 0; i < k; i ++)
		delete means[i];
	delete means;
	return group;
}

int* kmeans_p(int k, int NUM_DATA, int** data_input) {
	int* group = new int[NUM_DATA];
	double** means = new double* [k]; //K���� ����� ������ �迭 means�� �����

	for (int i = 0; i < k; i++)
		means[i] = new double[DIM]; //means�� K�� ��ŭ ���� �Ҵ�

	////  set new means  ////
	for (int i = 0; i < k; i++)
		for (int j = 0; j < DIM; j++)
			means[i][j] = data_input[i + 4][j];  //means�� ������ ���(�ƹ� ������)�� ä���


	////  clustering  ////
	#pragma omp parallel for shared(data_input, means, group) num_threads(THREAD_NUM)
	for (int p=omp_get_thread_num();p<INT_MAX;p+= THREAD_NUM) 
	{
	
		int flag = 0;

		//�����͸� write �ϴ� ������ �м��غ���
		//group[i]�� write�ϰ�, ������ �κ��� read�� �Ѵ�.
		#pragma omp parallel for shared(data_input, means, group) num_threads(THREAD_NUM)
		for(int i=0;i<NUM_DATA;i++)
		{
			double min_dis = MIN_DIS;
			int min = -1;

			// calculating distances //
			for (int j = 0; j < k; j++) {
				double dis = 0;
				for (int l = 0; l < DIM; l++)
					dis += pow(float(data_input[i][l] - means[j][l]), DIM); // means�� data ���� �Ÿ��� �缭 ���� ����� �Ÿ��� ����
				if (dis < min_dis) {
					min_dis = dis;
					min = j; //���� ����� means(���)�� �ε����� ������
				}
			}

			// set groups //
			group[i] = min; //���� ����� ��հ� ���� �׷쿡 ���� 
		}

		// allocate new memory for calculate //
		double** temp = new double* [k];
		int* count = new int[k];
		// k * dim �� ũ�Ⱑ �۱⶧���� ����ȭ�ϴ� �̵��� ����.
		for (int i = 0; i < k; i++) {
			count[i] = 0;
			temp[i] = new double[DIM];
			for (int j = 0; j < DIM; j++)
				temp[i][j] = 0;
		}

		// calculating new means // //���ο� ����� ���ϰ� ����� �ϳ��� �ٸ��ٸ� �ٽ� ���
		//NUM_DATA�� ���� ū ��ŭ �̵��� ����
	#pragma omp parallel for shared(count, group) num_threads(THREAD_NUM)
		for (int i = 0; i < NUM_DATA; i++) {
			count[group[i]]++;
			for (int j = 0; j < DIM; j++)
				temp[group[i]][j] += data_input[i][j];
		}

		for (int i = 0; i < k; i++)
			for (int j = 0; j < DIM; j++) {
				temp[i][j] /= count[i]; //���� ����� ��հ�
				if (fabs(temp[i][j] - means[i][j]) > ERROR_RANGE) { //fabs()<-���� ó�� �Լ�
					flag++; //����� �ٸ� ��� flag++;
					means[i][j] = temp[i][j];
				}
			}

		// break point //
		if (flag == 0) //����� ���� �ߴµ� ����� �ٸ��� ���� ��� ���ѷ��� break
			break;
	}

	////  freeing memory  ////
	for (int i = 0; i < k; i++)
		delete means[i];
	delete means;
	return group;
}