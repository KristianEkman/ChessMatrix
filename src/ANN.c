#include "ANN.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <Windows.h>
#include <math.h>

HANDLE ComputeLock;
void NewAnn() {
	ComputeLock = CreateMutex(NULL, FALSE, NULL);
	for (int i = 0; i < INPUT_SIZE; i++)
	{
		Neuron_I* inputNeuron = &Ann.Inputs[i];
		char inpBias = i == INPUT_SIZE - 1;
		inputNeuron->Value = inpBias ? 1 : 0;

		for (int h = 0; h < HIDDEN_SIZE; h++)
		{
			Neuron_H* pHiddenNeuron = &Ann.Hidden[h];
			Weight_I_H* pWeightI_H = &inputNeuron->Weights[h];

			char hidBias = h == HIDDEN_SIZE - 1;

			pHiddenNeuron->Value = hidBias ? 1 : 0;
			if (inpBias && hidBias) //no connection between biases
				continue;

			pWeightI_H->ConnectedNeuron = pHiddenNeuron;
			pWeightI_H->Value = ((double)rand() / (RAND_MAX)) - 1;

			for (int o = 0; o < OUTPUT_SIZE; o++)
			{
				Neuron_O* pOutputNeuron = &Ann.Output[o];
				Weight_H_O* pWeightH_O = &pHiddenNeuron->Weights[o];

				pWeightH_O->ConnectedNeuron = pOutputNeuron;
				pWeightH_O->Value = ((double)rand() / (RAND_MAX)) - 1;
				pOutputNeuron->Value = 0;
			}
		}
	}

}

void PrintAnn() {
	printf("Input\n");
	for (size_t i = 0; i < INPUT_SIZE; i++)
	{
		Neuron_I* neuron = &Ann.Inputs[i];
		printf("%f", neuron->Value);
		for (int n = 0; n < HIDDEN_SIZE; n++)
		{
			Weight_I_H* weight = &neuron->Weights[n];
			printf("\t%f -> %f, ", weight->Value, weight->ConnectedNeuron->Value);
		}
		printf("\n");
	}
	printf("Hidden\n");
	for (size_t i = 0; i < HIDDEN_SIZE; i++)
	{
		Neuron_H* neuron = &Ann.Hidden[i];
		printf("%f", neuron->Value);
		for (int n = 0; n < OUTPUT_SIZE; n++)
		{
			Weight_H_O* weight = &neuron->Weights[n];
			printf("\t%f -> %f, ", weight->Value, weight->ConnectedNeuron->Value);
		}
		printf("\n");
	}
	printf("Otput\n");
	for (size_t i = 0; i < OUTPUT_SIZE; i++)
	{
		Neuron_O* neuron = &Ann.Output[i];
		printf("%f\n", neuron->Value);
	}
}

void PrintOutput() {
	for (size_t i = 0; i < OUTPUT_SIZE; i++)
	{
		Neuron_O* neuron = &Ann.Output[i];
		printf("%f ", neuron->Value);
	}

	printf("   %f", Ann.TotalError);
	printf("\n");
}

double LeakyReLU(double x)
{
	if (x >= 0)
		return x;
	else
		return x / 20;
}

double Sigmoid(double x)
{
	double s = 1 / (1 + exp(-x));
	return s;
}

double Compute(double* data, int dataLength) {

	DWORD waitResult = WaitForSingleObject(ComputeLock, INFINITE);
	if (waitResult != WAIT_OBJECT_0) {
		printf("Unexpected wait for lock result: %d", waitResult);
		return;
	}

	if (dataLength != INPUT_SIZE - 1)
	{
		fprintf(stderr, "Length of input data is not same as Length of input layer.\n");
		exit(1000);
	}

	for (size_t i = 0; i < dataLength; i++)
		Ann.Inputs[i].Value = data[i];

	//Forward propagation
	for (size_t n = 0; n < INPUT_SIZE; n++)
	{
		Neuron_I* neuron = &Ann.Inputs[n];
		for (int i = 0; i < HIDDEN_SIZE - 1; i++)
		{
			Weight_I_H* weight = &neuron->Weights[i];
			weight->ConnectedNeuron->Value += (weight->Value * neuron->Value);
		}
	}

	// Applying aktivation function to all neurons in hidden layer except bias
	for (size_t n = 0; n < HIDDEN_SIZE - 1; n++)
	{
		Neuron_H* neuron = &Ann.Hidden[n];
		neuron->Value = LeakyReLU(neuron->Value / (double)(INPUT_SIZE)); // dividing to prevent over flow.
		//neuron.Value = Sigmoid(neuron.Value / Layers[l].Count);
	}

	//Next layer
	for (size_t n = 0; n < HIDDEN_SIZE; n++)
	{
		Neuron_H* neuron = &Ann.Hidden[n];
		for (size_t i = 0; i < OUTPUT_SIZE; i++)
		{
			Weight_H_O* weight = &neuron->Weights[i];
			weight->ConnectedNeuron->Value += (weight->Value * neuron->Value);
		}
	}

	// Applying aktivation function to all neurons in outputlayer layer
	for (int n = 0; n < OUTPUT_SIZE; n++)
	{
		Neuron_O* neuron = &Ann.Output[n];
		neuron->Value = LeakyReLU(neuron->Value / (double)(HIDDEN_SIZE)); // dividing to prevent over flow.
		//neuron.Value = Sigmoid(neuron.Value / Layers[l].Count);
	}
	
	ReleaseMutex(ComputeLock);
	return Ann.Output[0].Value;
}

void BackProp(double* targets, int targLength) {
	DWORD waitResult = WaitForSingleObject(ComputeLock, INFINITE);
	if (waitResult != WAIT_OBJECT_0) {
		printf("Unexpected wait for lock result: %d", waitResult);
		return;
	}

	if (targLength != OUTPUT_SIZE)
	{
		fprintf(stderr, "Length of target data is not same as Length of output layer.\n");
		exit(1000);
	}


	Ann.TotalError = 0;
	//backwards propagation
	for (int n = 0; n < OUTPUT_SIZE; n++)
	{
		Neuron_O* neuron = &Ann.Output[n];
		neuron->Target = targets[n];
		neuron->Error = pow(neuron->Target - neuron->Value, 2) / 2;
		neuron->Delta = (neuron->Value - neuron->Target) * (neuron->Value > 0 ? 1 : 1 / (double)20);
		//neuron.Delta = (neuron->Value - neuron->Target) * (neuron->Value * (1 - neuron->Value));
		Ann.TotalError += neuron->Error;
	}

	for (int i = 0; i < HIDDEN_SIZE; i++)
	{
		// This can be done in parallell.
		Neuron_H* neuron = &Ann.Hidden[i];
		for (int w = 0; w < OUTPUT_SIZE; w++)
		{
			Weight_H_O* weight = &neuron->Weights[w];
			weight->Delta = neuron->Value * weight->ConnectedNeuron->Delta;
		}
	}

	//Input Layer
	for (int i = 0; i < INPUT_SIZE; i++)
	{
		//Could be run in parallell
		Neuron_I* neuron = &Ann.Inputs[i];
		for (int w = 0; w < HIDDEN_SIZE - 1; w++)
		{
			Weight_I_H* weight = &neuron->Weights[w];
			for (size_t c = 0; c < OUTPUT_SIZE; c++)
			{
				Weight_H_O* connectedWeight = &weight->ConnectedNeuron->Weights[c];
				weight->Delta += connectedWeight->Value * connectedWeight->ConnectedNeuron->Delta;
			}
			double cv = weight->ConnectedNeuron->Value;
			weight->Delta *= cv > 0 ? 1 : 1 / (double)20;
			weight->Delta *= neuron->Value;
		}
	}

	//All deltas are done. Now calculate new weights.
	for (size_t n = 0; n < INPUT_SIZE; n++)
	{
		Neuron_I* neuron = &Ann.Inputs[n];
		for (size_t w = 0; w < HIDDEN_SIZE - 1; w++)
		{
			Weight_I_H* weight = &neuron->Weights[w];
			weight->Value -= (weight->Delta * Ann.LearnRate);
		}
	}

	for (size_t n = 0; n < HIDDEN_SIZE; n++)
	{
		Neuron_H* neuron = &Ann.Hidden[n];
		for (size_t w = 0; w < OUTPUT_SIZE; w++)
		{
			Weight_H_O* weight = &neuron->Weights[w];
			weight->Value -= (weight->Delta * Ann.LearnRate);
		}
	}
	ReleaseMutex(ComputeLock);
}