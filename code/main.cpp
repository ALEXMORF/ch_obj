#define CH_OBJ_IMPLEMENTATION
#include "ch_obj.h"

#include <stdio.h>
#include <time.h>

int main()
{
    printf("******CORRECTNESS TEST: LOAD cube.obj*****\n\n");
    vertex *Cube = LoadObj("../data/cube");
    
    for (int VertexIndex = 0; VertexIndex < BufCount(Cube); ++VertexIndex)
    {
        printf("vertex %d: \n", VertexIndex);
        printf("P <%.2f, %.2f, %.2f>: \n", 
               Cube[VertexIndex].P[0],
               Cube[VertexIndex].P[1],
               Cube[VertexIndex].P[2]);
        printf("N <%.2f, %.2f, %.2f>: \n", 
               Cube[VertexIndex].N[0],
               Cube[VertexIndex].N[1],
               Cube[VertexIndex].N[2]);
        printf("Albedo <%.2f, %.2f, %.2f>: \n", 
               Cube[VertexIndex].Albedo[0],
               Cube[VertexIndex].Albedo[1],
               Cube[VertexIndex].Albedo[2]);
        printf("Emission <%.2f, %.2f, %.2f>: \n", 
               Cube[VertexIndex].Emission[0],
               Cube[VertexIndex].Emission[1],
               Cube[VertexIndex].Emission[2]);
        printf("\n\n");
    }
    
    printf("*******CORRECTNESS TEST DONE********\n\n");
    
    printf("******PERFORMANCE TEST: LOAD buddha.obj*****\n\n");
    
    clock_t BeginLoad = clock();
    vertex *Buddha = LoadObj("../data/buddha");
    clock_t EndLoad = clock();
    float SecondsTook = (float)(EndLoad - BeginLoad) / (float)CLOCKS_PER_SEC;
    printf("loaded %d vertices\n", (int)BufCount(Buddha));
    printf("Time took %.2f seconds\n", SecondsTook);
    printf("Load rate: %.2f vertices/second (rough estimate)\n", (float)BufCount(Buddha) / SecondsTook);
    printf("\n");
    
    printf("*******PERFORMANCE TEST DONE*******\n\n");
    
    getchar();
    
    return 0;
}