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

	// CpGnuplot의 생성자에서 인자로 wgnuplot.exe의 전체 경로를 넘겨준다.
	// Gnuplot을 설치한 경로에 따라 이 값을 바꿔야 한다.
	CpGnuplot plot("C:\\Program Files\\gnuplot\\bin\\wgnuplot.exe");

	DS_timer timer(2);

	printf("사용하는 코어의 수 : %d\n", THREAD_NUM);
	timer.setTimerName(0, (char*)"[Serial]");
	timer.setTimerName(1, (char*)"[Parallel] Version 1");
	int** data_input = readfile();
	printf("데이터의 개수 : %d\n", NUM_DATA);
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
	
	plot.cmd("set term wxt 0"); //gnuplot 0번 윈도우로 원본 데이터 시각화
	plot.cmd("set title \"Original data \"");
	plot.cmd("plot'C:\\temp\\out_data_origin.dat' notitle with points");

	plot.cmd("set term wxt 1"); //gnuplot 1번 윈도우로 serial 처리 후 데이터 시각화
	plot.cmd("set title \"Serial \"");
	plot.cmd("set palette model RGB defined(0 'red', 1 'green',2 'blue')");
	plot.cmd("plot'C:\\temp\\out_data_serial.dat' u 1:2:($3) notitle with points palette");

	plot.cmd("set term wxt 2"); //gnu plot 2번 윈도우로 Parallel 처리 후 데이터 시각화
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

	//int data_input[NUM_DATA][DIM]; //2차원 데이터를 사용하니까 [][2]로 선언함
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
	double ** means = new double*[k]; //K개의 평균을 저장할 배열 means를 만든다

	for(int i = 0; i < k; i++)
		means[i] = new double[2]; //means를 K개 만큼 동적 할당

	////  set new means  ////
	for(int i = 0; i < k; i++) 
		for(int j = 0; j < 2; j++)
			means[i][j] = data_input[i+4][j];  //means에 임의의 평균(아무 데이터)을 채운다
	

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
					dis += pow(float(data_input[i][l] - means[j][l]),2); // means와 data 간의 거리를 재서 가장 가까운 거리를 구함
				if(dis < min_dis) {
					min_dis = dis;
					min = j; //가장 가까운 means(평균)의 인덱스를 저장함
				}
			}

			// set groups //
			group[i] = min; //가장 가까운 평균과 같은 그룹에 저장 
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

		// calculating new means // //새로운 평균을 구하고 평균이 하나라도 다르다면 다시 계산
		for(int i = 0; i < NUM_DATA; i++) {
			count[group[i]]++;
			for(int j = 0; j < 2; j++)
				temp[group[i]][j] += data_input[i][j];
		}

		for(int i = 0; i < k; i++)
			for(int j = 0; j < 2; j++) {
				temp[i][j] /= count[i]; //새로 계산한 평균값
				if(fabs(temp[i][j] - means[i][j]) > 0.0001) { //fabs()<-절댓값 처리 함수
					flag++; //평균이 다를 경우 flag++;
					means[i][j] = temp[i][j];
				}
			}

		// break point //
		if(flag == 0) //평균을 재계산 했는데 평균이 다르지 않은 경우 무한루프 break
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
	double** means = new double* [k]; //K개의 평균을 저장할 배열 means를 만든다

	for (int i = 0; i < k; i++)
		means[i] = new double[DIM]; //means를 K개 만큼 동적 할당

	////  set new means  ////
	for (int i = 0; i < k; i++)
		for (int j = 0; j < DIM; j++)
			means[i][j] = data_input[i + 4][j];  //means에 임의의 평균(아무 데이터)을 채운다


	////  clustering  ////
	#pragma omp parallel for shared(data_input, means, group) num_threads(THREAD_NUM)
	for (int p=omp_get_thread_num();p<INT_MAX;p+= THREAD_NUM) 
	{
	
		int flag = 0;

		//데이터를 write 하는 영역을 분석해보면
		//group[i]만 write하고, 나머지 부분은 read만 한다.
		#pragma omp parallel for shared(data_input, means, group) num_threads(THREAD_NUM)
		for(int i=0;i<NUM_DATA;i++)
		{
			double min_dis = MIN_DIS;
			int min = -1;

			// calculating distances //
			for (int j = 0; j < k; j++) {
				double dis = 0;
				for (int l = 0; l < DIM; l++)
					dis += pow(float(data_input[i][l] - means[j][l]), DIM); // means와 data 간의 거리를 재서 가장 가까운 거리를 구함
				if (dis < min_dis) {
					min_dis = dis;
					min = j; //가장 가까운 means(평균)의 인덱스를 저장함
				}
			}

			// set groups //
			group[i] = min; //가장 가까운 평균과 같은 그룹에 저장 
		}

		// allocate new memory for calculate //
		double** temp = new double* [k];
		int* count = new int[k];
		// k * dim 의 크기가 작기때문에 병렬화하는 이득이 없음.
		for (int i = 0; i < k; i++) {
			count[i] = 0;
			temp[i] = new double[DIM];
			for (int j = 0; j < DIM; j++)
				temp[i][j] = 0;
		}

		// calculating new means // //새로운 평균을 구하고 평균이 하나라도 다르다면 다시 계산
		//NUM_DATA의 수가 큰 만큼 이득이 생김
	#pragma omp parallel for shared(count, group) num_threads(THREAD_NUM)
		for (int i = 0; i < NUM_DATA; i++) {
			count[group[i]]++;
			for (int j = 0; j < DIM; j++)
				temp[group[i]][j] += data_input[i][j];
		}

		for (int i = 0; i < k; i++)
			for (int j = 0; j < DIM; j++) {
				temp[i][j] /= count[i]; //새로 계산한 평균값
				if (fabs(temp[i][j] - means[i][j]) > ERROR_RANGE) { //fabs()<-절댓값 처리 함수
					flag++; //평균이 다를 경우 flag++;
					means[i][j] = temp[i][j];
				}
			}

		// break point //
		if (flag == 0) //평균을 재계산 했는데 평균이 다르지 않은 경우 무한루프 break
			break;
	}

	////  freeing memory  ////
	for (int i = 0; i < k; i++)
		delete means[i];
	delete means;
	return group;
}