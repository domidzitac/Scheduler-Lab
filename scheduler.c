#ifdef _MSC_VER
#define _CRT_SECURE_NO_WARNINGS
#endif

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#define RNF "random-numbers.txt"
#define FILENAMELENGTH 256
#define RR_QUANTUM 2




FILE * RN, *inputFile;



enum ProcessState
{
	Unstarted,
	Ready,
	Running,
	Blocked,
	Terminated
};

char* processStateText[] = { "Unstarted",
	"ready",
	"running",
	"blocked",
	"terminated" };


char* algos[] =
{
	"First Come First Served",
	"Highest Penalty Ratio Next",
	"Round Robbin",
	"Shortest Job First"
};
enum Algos
{
	FCFS,
	HPRN,
	RR,
	SJF
};

char* text1 = "This detailed printout gives the state and remaining burst for each process";

typedef struct
{
	enum ProcessState state;
	int remainingCycles;
}Cycle;

typedef struct
{
	int finishingTime;
	double CPUUtilization;
	double IOUtilization;
	double throughput;
	double averageTurnAroundTime;
	double averageWaitingTime;
}SummaryData;

typedef struct
{
	int A; // arrival time
	int B; // CPU burst times uniformly distributed random integers (UDRI) in (0, B]
	int C; // total CPU needed
	int M; // multiplier, used to compute I/O burst time; I/O burst time = M * preceding CPU burst time
	int finishingTime;
	int turnAroundTime; // finishing time - A
	int IOTime; // time in blocked state
	int waitingTime; // ALL time in ready state
	int currentWaitingTime; // time in ready state since last run
	enum ProcessState processState;
	int CurrentCPUBurstTime;
	int CPUBurstTime;
	int CurrentIOBurstTime;
	int IOBurstTime;
	int CPUTimeRemaining;
	double Priority;
	int quantum;
	int CurrentQuantumCPUBurst;
} Process;


Process** processes;

// obtain UDRI t in some interval (0, U]
int randomOS(int U)
{
	int X;

	fscanf(RN, "%d", &X);

	return 1 + (X % U);
}

char inputFileName[FILENAMELENGTH];
int verbose = 0;
void printUsage()
{
	printf("Usage:\n");
	printf("<program name> <input-filename>\n");
	printf("<program name> --verbose <input-filename>\n");
}


int n; // number of processes.

void printOriginalInput()
{
	printf("The original input was: ");
	printf("%d ", n);
	for (int i = 0; i < n; i++)
		printf("(%d %d %d %d) ", processes[i]->A, processes[i]->B, processes[i]->C, processes[i]->M);

	printf("\n");
}

void sortByArrivalTime()
{
	Process* aux;
	for (int i = 0; i < n - 1; i++)
		for (int j = i + 1; j < n; j++)
			if (processes[i]->A > processes[j]->A)
			{
				aux = processes[i];
				processes[i] = processes[j];
				processes[j] = aux;
			}
}


void printSortedInput()
{
	printf("The (sorted) input is: ");
	printf("%d ", n);
	for (int i = 0; i < n; i++)
		printf("(%d %d %d %d) ", processes[i]->A, processes[i]->B, processes[i]->C, processes[i]->M);

	printf("\n\n");
}

int AllProcessesFinished()
{
	for (int i = 0; i < n; i++)
		if (processes[i]->processState != Terminated)
			return 0;

	return 1;
}


void printCycleDetail(int cycle, int alg)
{
	printf("Before cycle\t%d:", cycle);
	for (int i = 0; i < n; i++)
	{
		printf("%12s", processStateText[processes[i]->processState]);
		switch (processes[i]->processState)
		{
		case Unstarted: case Ready: case Terminated:
			printf("%3d", 0);
			break;
		case Running:
			if (alg != RR)
				printf("%3d", processes[i]->CurrentCPUBurstTime);
			else
				printf("%3d", processes[i]->CurrentQuantumCPUBurst);
			break;
		case Blocked:
			printf("%3d", processes[i]->CurrentIOBurstTime);
			break;
		}
	}
	printf(".\n");

}

void scheduleProcess(int i, enum Algos alg)
{
	if (alg != RR)
	{
		processes[i]->CPUBurstTime = randomOS(processes[i]->B);
		if (processes[i]->CPUBurstTime > processes[i]->CPUTimeRemaining)
		{
			processes[i]->CPUBurstTime = processes[i]->CPUTimeRemaining;
		}
		processes[i]->CurrentCPUBurstTime = processes[i]->CPUBurstTime;
		processes[i]->IOBurstTime = processes[i]->CPUBurstTime * processes[i]->M;
		processes[i]->CurrentIOBurstTime = processes[i]->IOBurstTime;


		processes[i]->IOTime += processes[i]->IOBurstTime;

	}
	else
	{
		if (processes[i]->processState == Blocked
			|| processes[i]->processState == Unstarted
			|| (processes[i]->processState == Ready && processes[i]->CurrentCPUBurstTime == 0))
		{
			processes[i]->CPUBurstTime = randomOS(processes[i]->B);
			if (processes[i]->CPUBurstTime > processes[i]->CPUTimeRemaining)
			{
				processes[i]->CPUBurstTime = processes[i]->CPUTimeRemaining;
			}
			processes[i]->CurrentCPUBurstTime = processes[i]->CPUBurstTime;
			processes[i]->IOBurstTime = processes[i]->CPUBurstTime * processes[i]->M;
			processes[i]->CurrentIOBurstTime = processes[i]->IOBurstTime;


			processes[i]->IOTime += processes[i]->IOBurstTime;

			processes[i]->quantum = processes[i]->CurrentQuantumCPUBurst = RR_QUANTUM;

			if (processes[i]->CPUBurstTime < processes[i]->CurrentQuantumCPUBurst)
				processes[i]->CurrentQuantumCPUBurst = processes[i]->CPUBurstTime;
		}
		else if ((processes[i]->processState == Ready || processes[i]->processState == Running)
			&& processes[i]->CurrentCPUBurstTime > 0)
		{
			processes[i]->quantum = processes[i]->CurrentQuantumCPUBurst = RR_QUANTUM;
			if (processes[i]->processState == Running)
			{
				processes[i]->CurrentCPUBurstTime--;
				processes[i]->CPUTimeRemaining--;
			}

			if (processes[i]->CurrentCPUBurstTime < processes[i]->CurrentQuantumCPUBurst)
				processes[i]->CurrentQuantumCPUBurst = processes[i]->CurrentCPUBurstTime;
		}
	}

	processes[i]->processState = Running;
	processes[i]->currentWaitingTime = 0;

}


void printAllProcessSumary(enum Algos alg)
{
	printf("The scheduling algorithm used was %s\n\n", algos[alg]);
	for (int i = 0; i < n; i++)
	{
		printf("Process %d:\n", i);
		printf("\t(A,B,C,M) = (%d,%d,%d,%d)\n", processes[i]->A, processes[i]->B, processes[i]->C, processes[i]->M);
		printf("\tFinishing time: %d\n", processes[i]->finishingTime);
		printf("\tTurnaround time: %d\n", processes[i]->turnAroundTime);
		printf("\tI/O time: %d\n", processes[i]->IOTime);
		printf("\tWaiting time: %d\n", processes[i]->waitingTime);
	}
	printf("\n");
}

void printSummaryData(SummaryData data)
{
	printf("Summary Data:\n");
	printf("\tFinishing time: %d\n", data.finishingTime);
	printf("\tCPU Utilization: %.6lf\n", data.CPUUtilization);
	printf("\tI/O Utilization: %.6lf\n", data.IOUtilization);
	printf("\tThroughput: %.6lf processes per hundred cycles\n", data.throughput);
	printf("\tAverage turnaround time: %.6lf\n", data.averageTurnAroundTime);
	printf("\tAverage waiting time: %.6lf\n", data.averageWaitingTime);

	printf("\n");
}

int find_FCFS_Process(int cycle)
{
	int index = -1;
	double maxPriority = 0;

	for (int i = 0; i < n; i++)
		processes[i]->Priority = 0;

	for (int i = 0; i < n; i++)
	{
		if (processes[i]->processState == Blocked && processes[i]->CurrentIOBurstTime == 1)
			processes[i]->Priority = 1;
		else if (processes[i]->processState == Ready)
		{
			processes[i]->Priority = processes[i]->currentWaitingTime + 1;
		}
		else if (processes[i]->processState == Unstarted && cycle - processes[i]->A > 0)
			processes[i]->Priority = cycle - processes[i]->A;

	}

	for (int i = 0; i < n; i++)
	{
		if (processes[i]->Priority > maxPriority)
		{
			maxPriority = processes[i]->Priority;
			index = i;
		}
	}
	return index;
}

double Max(double a, double b)
{
	return (a > b) ? a : b;
}
int find_HPRN_Process(int cycle)
{
	int index = -1;
	double maxPriority = -1;

	for (int i = 0; i < n; i++)
		processes[i]->Priority = -1;

	for (int i = 0; i < n; i++)
	{
		if (processes[i]->processState == Blocked && processes[i]->CurrentIOBurstTime == 1)
			processes[i]->Priority = (double)(cycle - 1 - processes[i]->A) / Max(1, processes[i]->C - processes[i]->CPUTimeRemaining);
		else if (processes[i]->processState == Ready)
		{
			processes[i]->Priority = (double)(cycle - 1 - processes[i]->A) / Max(1, processes[i]->C - processes[i]->CPUTimeRemaining);
		}
		else if (processes[i]->processState == Unstarted && cycle - processes[i]->A > 0)
			processes[i]->Priority = (double)(cycle - 1 - processes[i]->A) / Max(1, processes[i]->C - processes[i]->CPUTimeRemaining);

	}

	for (int i = 0; i < n; i++)
	{
		if (processes[i]->Priority > maxPriority)
		{
			maxPriority = processes[i]->Priority;
			index = i;
		}
	}
	return index;
}

int find_RR_Process(int cycle)
{
	int index = -1;
	double maxPriority = 0;

	for (int i = 0; i < n; i++)
		processes[i]->Priority = 0;

	for (int i = 0; i < n; i++)
	{
		if (processes[i]->processState == Blocked && processes[i]->CurrentIOBurstTime == 1)
			processes[i]->Priority = 1;
		else if (processes[i]->processState == Ready)
		{
			processes[i]->Priority = processes[i]->currentWaitingTime + 1;
		}
		else if (processes[i]->processState == Unstarted && cycle - processes[i]->A > 0)
			processes[i]->Priority = cycle - processes[i]->A;
		else if (processes[i]->processState == Running && processes[i]->CurrentQuantumCPUBurst == 1
			&& processes[i]->CurrentCPUBurstTime > 1)
			processes[i]->Priority = 1;

	}

	for (int i = 0; i < n; i++)
	{
		if (processes[i]->Priority > maxPriority)
		{
			maxPriority = processes[i]->Priority;
			index = i;
		}
	}
	return index;
}

int find_SJF_Process(int cycle)
{
	int index = -1, minPriority = 0;

	for (int i = 0; i < n; i++)
		processes[i]->Priority = 0;

	for (int i = 0; i < n; i++)
	{
		if ((processes[i]->processState == Blocked && processes[i]->CurrentIOBurstTime == 1)
			|| (processes[i]->processState == Ready)
			|| (processes[i]->processState == Unstarted && cycle - processes[i]->A > 0)
			)
			processes[i]->Priority = processes[i]->CPUTimeRemaining;
	}

	for (int i = 0; i < n; i++)
	{
		if (processes[i]->Priority > 0)
			if (index == -1)
			{
				minPriority = processes[i]->Priority;
				index = i;
			}
			else if (processes[i]->Priority < minPriority)
			{
				minPriority = processes[i]->Priority;
				index = i;
			}

	}
	return index;
}
int findProcessToSchedule(enum Algos algo, int cycle)
{
	switch (algo)
	{
	case FCFS:
		return find_FCFS_Process(cycle);
		break;
	case HPRN:
		return find_HPRN_Process(cycle);
		break;
	case RR:
		return find_RR_Process(cycle);
		break;
	case SJF:
		return find_SJF_Process(cycle);
		break;
	default:
		break;
	}
	return -1;
}


void getSummaryData(SummaryData* data, int cyclesRunning, int cyclesBlocked)
{
	//1 . finishing time
	int max = 0;
	for (int i = 0; i < n; i++)
		if (processes[i]->finishingTime > max)
			max = processes[i]->finishingTime;
	data->finishingTime = max;

	// 2. CPU Utilization
	data->CPUUtilization = (double)cyclesRunning / data->finishingTime;


	// 3. I/O Utilization
	data->IOUtilization = (double)cyclesBlocked / data->finishingTime;
	// 4. Throughput
	data->throughput = 100 * (double)n / data->finishingTime;
	// 5. average turnaround time
	int sum = 0;
	for (int i = 0; i < n; i++)
		sum += processes[i]->turnAroundTime;
	data->averageTurnAroundTime = (double)sum / n;

	// 6. average waiting time
	sum = 0;
	for (int i = 0; i < n; i++)
		sum += processes[i]->waitingTime;
	data->averageWaitingTime = (double)sum / n;
}

void openRandomNumberFile()
{
	RN = fopen(RNF, "r");
	if (RN != NULL)
	{
		//printf("The file %s was opened\n", RNF);
	}
	else
	{
		printf("The file %s was not opened\n", RNF);
		exit(1);
	}
}

void processCmdLineArgs(int argc, char* argv[])
{
	if (argc == 1)
	{
		printf("Too few arguments.\n");
		printUsage();
		exit(1);
	}
	else if (argc > 3)
	{
		printf("Too many arguments.");
		printUsage();
		exit(0);
	}

	if (argc == 2)
	{
		strcpy(inputFileName, argv[1]);
		verbose = 0;
	}
	else if (argc == 3)
	{
		strcpy(inputFileName, argv[2]);
		verbose = 1;
	}

}



void readInput()
{
	int err;
	inputFile = fopen(inputFileName, "r");
	if (inputFile != NULL)
	{
		//printf("The file %s was opened\n", inputFileName);
	}
	else
	{
		printf("The file %s was not opened. Exiting...\n", inputFileName);
		exit(1);
	}


	// read Input and initialize data
	if (inputFile != NULL)
		fscanf(inputFile, "%d", &n);

	processes = (Process * *)malloc(sizeof(Process*) * n);
	for (int i = 0; i < n; i++)
	{
		processes[i] = (Process*)malloc(sizeof(Process));

		fscanf(inputFile, "%d %d %d %d", &processes[i]->A, &processes[i]->B, &processes[i]->C, &processes[i]->M);

	}
}

void resetData()
{
	for (int i = 0; i < n; i++)
	{
		processes[i]->finishingTime = 0;
		processes[i]->IOTime = 0;
		processes[i]->turnAroundTime = 0;
		processes[i]->waitingTime = 0;
		processes[i]->processState = Unstarted;
		processes[i]->CurrentCPUBurstTime = 0;
		processes[i]->CurrentIOBurstTime = 0;
		processes[i]->CPUBurstTime = 0;
		processes[i]->IOBurstTime = 0;
		processes[i]->CPUTimeRemaining = processes[i]->C;
		processes[i]->currentWaitingTime = 0;
		processes[i]->Priority = 0;

		processes[i]->quantum = 0;
		processes[i]->CurrentQuantumCPUBurst = 0;
	}
}

void sortByJobLength()
{
	Process* aux;
	for (int i = 0; i < n - 1; i++)
		for (int j = i + 1; j < n; j++)
			if (processes[i]->C > processes[j]->C)
			{
				aux = processes[i];
				processes[i] = processes[j];
				processes[j] = aux;
			}
}
void simulateScheduling(enum Algos alg)
{
	int cycle;
	int currentProcess = 0;
	int CPUIdle = 1, i;

	resetData();
	printOriginalInput();

	switch (alg)
	{
	case FCFS:
		sortByArrivalTime();
		break;
	case HPRN:
		sortByArrivalTime();
		break;
	case RR:
		sortByArrivalTime();
		break;
	case SJF:
		sortByArrivalTime();
		break;
	default:
		break;
	}


	printSortedInput();




	cycle = 0;





	if (verbose)
		printf("%s\n\n", text1);

	if (verbose)
		printCycleDetail(cycle, alg);
	int sched;
	int cyclesCPURunning = 0; // number of cycles at least one process is runnging
	int cyclesIOUsed = 0; // number o cycles at least one process is blocked (using I/O)

	while (!AllProcessesFinished())
	{
		cycle++;

		for (i = 0; i < n; i++)
		{
			if (processes[i]->processState == Running && processes[i]->CurrentCPUBurstTime == 1)
				CPUIdle = 1;
			if (alg == RR)
				if (processes[i]->processState == Running && processes[i]->CurrentQuantumCPUBurst == 1)
					CPUIdle = 1;
		}
		sched = -1;
		if (CPUIdle == 1)
			sched = findProcessToSchedule(alg, cycle);

		if (CPUIdle == 1 && sched >= 0)
		{
			CPUIdle = 0;
			scheduleProcess(sched, alg);
		}



		for (i = 0; i < n; i++)
		{
			switch (processes[i]->processState)
			{
			case Unstarted:
				if (processes[i]->A >= cycle)
					// stay in Unstarted state
					;
				else if (CPUIdle == 1)
				{
					//CPUIdle = 0;
					//scheduleProcess(i);
					//processes[i]->currentWaitingTime == 0;
				}
				else
				{
					processes[i]->processState = Ready;
					processes[i]->waitingTime++;
					processes[i]->currentWaitingTime++;
				}
				break;
			case Ready:

				processes[i]->waitingTime++;
				processes[i]->currentWaitingTime++;


				break;
			case Running:
				if (i != sched)
				{
					if (processes[i]->CurrentCPUBurstTime > 1)
					{
						processes[i]->CurrentCPUBurstTime--;
						processes[i]->CPUTimeRemaining--;
					}
					else if (processes[i]->CurrentCPUBurstTime == 1)
					{
						if (sched == -1)
							CPUIdle = 1;
						processes[i]->CurrentCPUBurstTime--;
						processes[i]->CPUTimeRemaining--;
						if (processes[i]->CPUTimeRemaining > 0)
						{
							processes[i]->processState = Blocked;
							//processes[i]->IOTime++;
						}
						else if (processes[i]->CPUTimeRemaining == 0)
						{
							processes[i]->processState = Terminated;
							processes[i]->IOTime -= processes[i]->IOBurstTime;
							processes[i]->finishingTime = cycle - 1;
							processes[i]->turnAroundTime = processes[i]->finishingTime - processes[i]->A;
						}
					}

					if (alg == RR)
					{
						processes[i]->CurrentQuantumCPUBurst--;
						if (processes[i]->CurrentQuantumCPUBurst == 0)
							if (processes[i]->CurrentCPUBurstTime > 0)
							{
								processes[i]->processState = Ready;
								processes[i]->waitingTime++;
								processes[i]->currentWaitingTime = 1;
							}


					}


				}
				else
				{
					if (alg == RR)
					{
						/// ????

					}
				}
				break;
			case Blocked:
				if (processes[i]->CurrentIOBurstTime > 1)
				{
					processes[i]->CurrentIOBurstTime--;
					//processes[i]->IOTime++;
				}

				else if (processes[i]->CurrentIOBurstTime == 1)
				{
					//processes[i]->IOTime++;
					if (CPUIdle == 0)
					{
						processes[i]->processState = Ready;
						processes[i]->waitingTime++;
						processes[i]->currentWaitingTime++;
					}
					else
					{
						//CPUIdle = 0;
						//scheduleProcess(i);
						//processes[i]->currentWaitingTime == 0;
					}
				}
				break;
			case Terminated:
				//nothing left to do
				break;
			}
		}



		if (verbose && !AllProcessesFinished())
			printCycleDetail(cycle, alg);

		for (int k = 0; k < n; k++)
			if (processes[k]->processState == Running)
			{
				cyclesCPURunning++;
				break;
			}
		for (int k = 0; k < n; k++)
			if (processes[k]->processState == Blocked)
			{
				cyclesIOUsed++;
				break;
			}
	}

	printAllProcessSumary(alg);

	SummaryData summary;
	getSummaryData(&summary, cyclesCPURunning, cyclesIOUsed);

	printSummaryData(summary);
}


int main(int argc, char* argv[])
{

	// Open random numbers file
	openRandomNumberFile();


	// process comand line arguments
	processCmdLineArgs(argc, argv);

	// Open/Read input file

	readInput();


	//printf(inputFileName);


	// First Come First Served
	simulateScheduling(FCFS);
	if (RN)
	{
		fclose(RN);
	}




	// Round Robin
	openRandomNumberFile();
	simulateScheduling(RR);
	if (RN)
	{
		fclose(RN);
	}

	// Shortest Job First
	openRandomNumberFile();
	simulateScheduling(SJF);
	if (RN)
	{
		fclose(RN);
	}

	// HPRN
	openRandomNumberFile();
	simulateScheduling(HPRN);



	// close files
	if (RN)
	{
		fclose(RN);
	}

	if (inputFile)
	{
		fclose(inputFile);
	}

	return 0;
}