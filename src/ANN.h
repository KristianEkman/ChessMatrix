#pragma once
#define INPUT_SIZE 69 + 1
#define HIDDEN_SIZE 1000 + 1
#define OUTPUT_SIZE 1

typedef struct Neuron_O {
	double Value;
	double Target;
	double Error;
	double Delta;
} Neuron_O;

typedef struct Weight_H_O {
	double Value;
	double Delta;
	struct Neuron_O* ConnectedNeuron;
} Weight_H_O;

typedef struct Neuron_H {
	double Value;
	struct Weight_H_O Weights[OUTPUT_SIZE];
} Neuron_H;


typedef struct Weight_I_H {
	double Value;
	double Delta;
	struct Neuron_H* ConnectedNeuron;
} Weight_I_H;


typedef struct Neuron_I {
	double Value;
	struct Weight_I_H Weights[HIDDEN_SIZE];
} Neuron_I;

typedef struct {
	Neuron_I Inputs[INPUT_SIZE];
	Neuron_H Hidden[HIDDEN_SIZE];
	Neuron_O Output[OUTPUT_SIZE];

	double TotalError;
	double LearnRate;
} ANN;

ANN Ann;

void NewAnn();
void PrintAnn();
double Compute(double* data, int dataLength);
void BackProp(double* targets, int targLength);
void PrintOutput();
