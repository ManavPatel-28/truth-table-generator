#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include<stdbool.h>
#include<stdarg.h>

#define MAX_INPUTS 100
#define MAX_OUTPUTS 100
#define MAX_COMPONENTS 200
#define MAX_VARS 500

typedef struct 
{
    char type[16];
    char inputs[MAX_VARS][16];
    char output[16];
    int numInputs;
    int numSelect;

} Component;


typedef struct 
{
    int numInputs;
    int numOutputs;
    char inputs[MAX_INPUTS][16];
    char outputs[MAX_OUTPUTS][16];
    Component components [MAX_COMPONENTS];
    int numComponents;
    char tempLabels[MAX_VARS][16];
    int tempVales[MAX_VARS];
    int numTemp;

} Circut;

bool debugEnabled = false;

void debugMessage(const char *format, ...)
{

    if(!debugEnabled) return;
    va_list args;
    va_start(args, format);
    vfprintf(stderr, format, args);
    fprintf(stderr, "\n");
    va_end(args);

}

int getTempIndex(Circut *circuit, const char *label)
{
    for(int i = 0; i < circuit->numTemp; i++){
        if(strcmp(circuit->tempLabels[i], label) == 0) {
            return i;
        }
    }
    if(circuit->numTemp >= MAX_VARS) {
        fprintf(stderr, "Error: Too many temporaty variables.\n");
        exit(EXIT_FAILURE);
    }
    strcpy(circuit->tempLabels[circuit->numTemp], label);
    return circuit->numTemp++;

    
}


int getValue(Circut *circuit, const char *label, const int *values)
{
    if(strcmp(label, "0") == 0) return 0;
    if(strcmp(label, "1") == 0) return 1;

    for(int i = 0; i < circuit->numInputs; i++)
    {
        if(strcmp(label, circuit->inputs[i]) == 0)
        {
            return values[i];
        }
    }

    int tempIndex = getTempIndex(circuit, label);
    return circuit->tempVales[tempIndex];



}

void setValue(Circut *circuit, const char *label, int value)
{
    if(strcmp(label, "0") == 0 || strcmp(label, "1") == 0) return;
    int tempIndex = getTempIndex(circuit, label);
    circuit->tempVales[tempIndex] = value;
}

void parseInputs(FILE *file, Circut *circuit){
    fscanf(file, " INPUT %d", &circuit->numInputs);
    for(int i = 0; i < circuit->numInputs; i++)
    {

        fscanf(file, " %s", circuit->inputs[i]);
    }
}

void parseOutputs(FILE *file, Circut *circuit){
    fscanf(file, " OUTPUT %d", &circuit->numOutputs);
    for(int i = 0; i < circuit->numOutputs; i++)
    {

        fscanf(file, " %s", circuit->outputs[i]);
    }
}

void parseComponents(FILE *file, Circut *circuit) 
{
    char type[16];
    while (fscanf(file, " %s", type) == 1)
     {
        Component *comp = &circuit->components[circuit->numComponents++];
        strcpy(comp->type, type);

        if (strcmp(type, "AND") == 0 || strcmp(type, "OR") == 0 ||
            strcmp(type, "NAND") == 0 || strcmp(type, "NOR") == 0 ||
            strcmp(type, "XOR") == 0) 
            {
            fscanf(file, " %s %s %s", comp->inputs[0], comp->inputs[1], comp->output);
            comp->numInputs = 2;
        } else if (strcmp(type, "NOT") == 0)
         {
            fscanf(file, " %s %s", comp->inputs[0], comp->output);
            comp->numInputs = 1;
        } else if (strcmp(type, "MULTIPLEXER") == 0)
         {
            int k;
            fscanf(file, " %d", &k);
            comp->numInputs = (1 << k) + k; 
            comp->numSelect = k;

            for (int i = 0; i < comp->numInputs; i++)
             {
                fscanf(file, " %s", comp->inputs[i]);
            }
            fscanf(file, " %s", comp->output);
        } else if (strcmp(type, "DECODER") == 0)
         {
            int numInputs;
            fscanf(file, " %d", &numInputs);
            comp->numInputs = numInputs;

            for (int i = 0; i < numInputs; i++)
             {
                fscanf(file, " %s", comp->inputs[i]);
            }

            int numOutputs = 1 << numInputs;  
            
            for (int i = 0; i < numOutputs; i++) {
                fscanf(file, " %s", comp->inputs[numInputs + i]);
            }
        } else if (strcmp(type, "PASS") == 0) 
        {
            fscanf(file, " %s %s", comp->inputs[0], comp->output);
            comp->numInputs = 1;
        } else {
            fprintf(stderr, "Error: Unsupported component type '%s'.\n", type);
            exit(EXIT_FAILURE);
        }
    }
}

int evaluateGate(Circut *circuit, const Component *comp, const int *values) {
    int result = 0;

    if (strcmp(comp->type, "AND") == 0) 
    {
        result = getValue(circuit, comp->inputs[0], values) & getValue(circuit, comp->inputs[1], values);
    } else if (strcmp(comp->type, "OR") == 0)
     {
        result = getValue(circuit, comp->inputs[0], values) | getValue(circuit, comp->inputs[1], values);
    } else if (strcmp(comp->type, "NOT") == 0)
     {
        result = !getValue(circuit, comp->inputs[0], values);
    } else if (strcmp(comp->type, "XOR") == 0) 
    {
        result = getValue(circuit, comp->inputs[0], values) ^ getValue(circuit, comp->inputs[1], values);
    } else if (strcmp(comp->type, "NAND") == 0)
     {
        result = !(getValue(circuit, comp->inputs[0], values) & getValue(circuit, comp->inputs[1], values));
    } else if (strcmp(comp->type, "NOR") == 0)
     {
        result = !(getValue(circuit, comp->inputs[0], values) | getValue(circuit, comp->inputs[1], values));
    } else if (strcmp(comp->type, "MULTIPLEXER") == 0)
     {
        int selectValue = 0;
        int numDataInputs = 1 << comp->numSelect;  
        for (int i = 0; i < comp->numSelect; i++) {
            selectValue |= getValue(circuit, comp->inputs[numDataInputs + i], values) << (comp->numSelect - 1 - i);
        }
        result = getValue(circuit, comp->inputs[selectValue], values);
    } else if (strcmp(comp->type, "DECODER") == 0) 
    
    {
        int inputValue = 0;
        for (int i = 0; i < comp->numInputs; i++) 
        {
            inputValue = (inputValue << 1) | getValue(circuit, comp->inputs[i], values);
        }

        int numOutputs = 1 << comp->numInputs;
        for (int i = 0; i < numOutputs; i++) 
        {
            setValue(circuit, comp->inputs[comp->numInputs + i], (i == inputValue) ? 1 : 0);
        }
    } else if (strcmp(comp->type, "PASS") == 0) 
    {
        result = getValue(circuit, comp->inputs[0], values);
    } else {
        fprintf(stderr, "Error: Unsupported gate type '%s'.\n", comp->type);
        exit(EXIT_FAILURE);
    }

    return result;
}

void generateTruthTable(Circut *circuit)
 {
    int numRows = 1 << circuit->numInputs;
    int values[MAX_VARS] = {0};

    for (int row = 0; row < numRows; row++) 
    {
        memset(circuit->tempVales, 0, sizeof(circuit->tempVales));
        circuit->numTemp = 0;

        for (int i = 0; i < circuit->numInputs; i++) 
        {
            values[i] = (row >> (circuit->numInputs - 1 - i)) & 1;
        }

        for (int i = 0; i < circuit->numComponents; i++)
         {
            const Component *comp = &circuit->components[i];
            int result = evaluateGate(circuit, comp, values);
            setValue(circuit, comp->output, result);
        }

        for (int i = 0; i < circuit->numInputs; i++)
         {
            printf("%d ", values[i]);
        }
        printf("|");

        for (int i = 0; i < circuit->numOutputs; i++)
         {
            int outputValue = getValue(circuit, circuit->outputs[i], values);
            printf(" %d", outputValue);
        }


        printf("\n");
    }
}



int main(int argc, char *argv[]) 
{
    if (argc != 2) 
    {
        fprintf(stderr, "Usage: %s <circuit_file>\n", argv[0]);
        return EXIT_FAILURE;
    }

    FILE *file = fopen(argv[1], "r");

    if (!file) 

    {
        perror("Error opening file");
        return EXIT_FAILURE;
    }

    Circut circuit = {0};

    parseInputs(file, &circuit);

    parseOutputs(file, &circuit);

    parseComponents(file, &circuit);


    generateTruthTable(&circuit);

    fclose(file);

    return EXIT_SUCCESS;
}