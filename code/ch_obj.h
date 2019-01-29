#pragma once

/*

NOTE(chen):

dependencies:

. ch_buf.h

 to use this library, include the file like this:
 
#define CH_OBJ_IMPLEMENTATION
#include "ch_obj.h"

*/

/*

TODO(chen):

. triangulate quad faces
. parse different material encoding
. add parallelism for faster speed

*/

//
//
// API

struct ch_material
{
    char Name[255];
    float Albedo[4];
    float Emission[3];
};

struct ch_vertex
{
    float P[3];
    float N[3];
    float Albedo[3];
    float Emission[3];
};

ch_vertex *CHLoadObj(char *Path);

//
//
// Implementation

#ifdef CH_OBJ_IMPLEMENTATION

#include <stdio.h>
#include <stdint.h>
#include <math.h>
#include <assert.h>
#include <string.h>
#include "ch_buf.h"

#define CH_ASSERT(Value) assert(Value)

static char *
CHReadFile(char *FilePath)
{
    char *Buffer = 0;
    
    FILE *File = fopen(FilePath, "rb");
    if (File)
    {
        fseek(File, 0, SEEK_END);
        int FileSize = ftell(File);
        rewind(File);
        
        Buffer = (char *)malloc(FileSize+1);
        Buffer[FileSize] = 0;
        fread(Buffer, 1, FileSize, File);
        
        fclose(File);
    }
    
    return Buffer;
}

inline char *
CHGotoNextLine(char *Ptr)
{
    while (*Ptr != '\n')
    {
        Ptr += 1;
    }
    
    while (*Ptr == '\r' || *Ptr == '\n')
    {
        Ptr += 1;
    }
    
    return Ptr;
}

inline int
CHLength(char *Str)
{
    int Count = 0;
    
    while (*Str)
    {
        Count += 1;
        Str += 1;
    }
    
    return Count;
}

inline bool
CHEqual(char *Str1, char *Str2)
{
    while (*Str1 && *Str2)
    {
        if (*Str1++ != *Str2++)
        {
            return false;
        }
    }
    return true;
}

inline bool
CHStartsWidth(char *Str, char *SubStr)
{
    for (int CharIndex = 0; CharIndex < CHLength(SubStr); ++CharIndex)
    {
        if (Str[CharIndex] != SubStr[CharIndex])
        {
            return false;
        }
    }
    
    return true;
}

inline char *
CHSkipSpaces(char *Cursor)
{
    char *EndCursor = Cursor;
    
    while (*EndCursor == ' ')
    {
        EndCursor += 1;
    }
    
    return EndCursor;
}

#define EXPECT_STR(Cursor, S) \
if (!CHStartsWidth(Cursor, S)) \
{ \
    return false; \
} \
Cursor += CHLength(S); \

#define EXPECT_CHAR(Cursor, C) \
if (*Cursor != C) \
{ \
    return false; \
} \
Cursor += 1; \

inline bool
CHParseString(char *Cursor, char *Prefix, char *Buffer_Out)
{
    Cursor = CHSkipSpaces(Cursor);
    
    EXPECT_STR(Cursor, Prefix);
    
    Cursor = CHSkipSpaces(Cursor);
    
    int BufferIndex = 0;
    while (*Cursor != '\n')
    {
        Buffer_Out[BufferIndex++] = *Cursor++;
    }
    
    return true;
}

inline char *
CHReadInteger(char *Str, int32_t *Integer_Out)
{
    char *Cursor = Str;
    Cursor = CHSkipSpaces(Cursor);
    
    int Sign = 1;
    if (*Cursor == '-')
    {
        Sign = -1;
        Cursor += 1;
    }
    
    int Integer = 0;
    while (*Cursor >= '0' && *Cursor <= '9')
    {
        int Digit = *Cursor++ - '0';
        Integer = Integer * 10 + Digit;
    }
    
    *Integer_Out = Sign * Integer;
    return Cursor;
}

inline char *
CHReadFloat(char *Str, float *Float_Out)
{
    char *Cursor = Str;
    Cursor = CHSkipSpaces(Cursor);
    
    float Sign = 1.0f;
    if (*Cursor == '-')
    {
        Sign = -1.0f;
        Cursor += 1;
    }
    
    int WholeNum = 0;
    float DeciNum = 0.0f;
    
    while (*Cursor >= '0' && *Cursor <= '9')
    {
        int Digit = *Cursor++ - '0';
        WholeNum = 10 * WholeNum + Digit;
    }
    
    if (*Cursor == '.')
    {
        Cursor += 1;
        
        float Weight = 0.1f;
        while (*Cursor >= '0' && *Cursor <= '9')
        {
            int Digit = *Cursor++ - '0';
            DeciNum += Weight * (float)Digit;
            Weight *= 0.1f;
        }
    }
    
    //NOTE(chen): scientific notation
    float E = 1.0f;
    if (*Cursor == 'e' || *Cursor == 'E')
    {
        Cursor += 1;
        
        int Exponent = 0;
        Cursor = CHReadInteger(Cursor, &Exponent);
        E = powf(10.0f, (float)Exponent);
    }
    
    *Float_Out = Sign * ((float)WholeNum + DeciNum) * E;
    return Cursor;
}

inline bool
CHParseFloat(char *Cursor, char *Prefix, float *Float_Out)
{
    Cursor = CHSkipSpaces(Cursor);
    EXPECT_STR(Cursor, Prefix);
    Cursor = CHSkipSpaces(Cursor);
    
    Cursor = CHReadFloat(Cursor, Float_Out);
    
    return true;
}

inline bool
CHParseV3(char *Cursor, char *Prefix, float V_Out[3])
{
    Cursor = CHSkipSpaces(Cursor);
    EXPECT_STR(Cursor, Prefix);
    Cursor = CHSkipSpaces(Cursor);
    
    for (int I = 0; I < 3; ++I)
    {
        Cursor = CHReadFloat(Cursor, V_Out + I);
    }
    
    return true;
}

inline bool 
CHParseFaceWithTexCoord(char *Cursor, int *VertexIndices, int *NormalIndices)
{
    Cursor = CHSkipSpaces(Cursor);
    EXPECT_CHAR(Cursor, 'f');
    
    for (int PointIndex = 0; PointIndex < 3; ++PointIndex)
    {
        Cursor = CHSkipSpaces(Cursor);
        
        int VertexIndex, TexIndex, NormalIndex;
        Cursor = CHReadInteger(Cursor, &VertexIndex);
        EXPECT_CHAR(Cursor, '/');
        Cursor = CHReadInteger(Cursor, &TexIndex);
        EXPECT_CHAR(Cursor, '/');
        Cursor = CHReadInteger(Cursor, &NormalIndex);
        
        VertexIndices[PointIndex] = VertexIndex;
        NormalIndices[PointIndex] = NormalIndex;
    }
    
    return true;
}

#undef EXPECT_STR
#undef EXPECT_CHAR

static ch_vertex *
CHLoadObj(char *Path)
{
    ch_vertex *Vertices = {};
    
    char MtlPath[255];
    snprintf(MtlPath, sizeof(MtlPath), "%s.mtl", Path);
    char ObjPath[255];
    snprintf(ObjPath, sizeof(ObjPath), "%s.obj", Path);
    
    ch_material *Mats = 0;
    
    char *MtlFileContent = CHReadFile(MtlPath);
    if (MtlFileContent)
    {
        char *MtlFileWalker = MtlFileContent;
        
        while (*MtlFileWalker)
        {
            if (CHStartsWidth(MtlFileWalker, "newmtl"))
            {
                ch_material NewMat = {};
                CHParseString(MtlFileWalker, "newmtl", NewMat.Name);
                BufPush(Mats, NewMat);
            }
            else if (CHStartsWidth(MtlFileWalker, "Kd"))
            {
                CH_ASSERT(BufCount(Mats) > 0);
                
                float Albedo3[3] = {};
                CHParseV3(MtlFileWalker, "Kd", Albedo3);
                BufLast(Mats).Albedo[0] = Albedo3[0];
                BufLast(Mats).Albedo[1] = Albedo3[1];
                BufLast(Mats).Albedo[2] = Albedo3[2];
                BufLast(Mats).Albedo[3] = 1.0f;
            }
            else if (CHStartsWidth(MtlFileWalker, "d"))
            {
                CH_ASSERT(BufCount(Mats) > 0);
                CHParseFloat(MtlFileWalker, "d", &BufLast(Mats).Albedo[3]);
            }
            else if (CHStartsWidth(MtlFileWalker, "Ke"))
            {
                CH_ASSERT(BufCount(Mats) > 0);
                CHParseV3(MtlFileWalker, "Ke", BufLast(Mats).Emission);
            }
            
            MtlFileWalker = CHGotoNextLine(MtlFileWalker);
        }
    }
    free(MtlFileContent);
    
    char *ObjFileContent = CHReadFile(ObjPath);
    if (!ObjFileContent)
    {
        return 0;
    }
    char *ObjFileWalker = ObjFileContent;
    
    float *TempVertices = 0;
    float *TempNormals = 0;
    
    Vertices = 0;
    int AlphaVertexCount = 0;
    
    int CurrentMatIndex = -1;
    ObjFileWalker = ObjFileContent;
    while (*ObjFileWalker)
    {
        if (CHStartsWidth(ObjFileWalker, "vn"))
        {
            float Normal[3] = {};
            CHParseV3(ObjFileWalker, "vn", Normal);
            
            BufPush(TempNormals, Normal[0]);
            BufPush(TempNormals, Normal[1]);
            BufPush(TempNormals, Normal[2]);
        }
        else if (CHStartsWidth(ObjFileWalker, "v"))
        {
            float Vertex[3] = {};
            CHParseV3(ObjFileWalker, "v", Vertex);
            
            BufPush(TempVertices, Vertex[0]);
            BufPush(TempVertices, Vertex[1]);
            BufPush(TempVertices, Vertex[2]);
        }
        else if (CHStartsWidth(ObjFileWalker, "f"))
        {
            //NOTE(chen): discards faces with alpha less than 1
            bool FaceIsTransparent = (CurrentMatIndex != -1 &&
                                      Mats[CurrentMatIndex].Albedo[3] < 0.9f);
            
            if (!FaceIsTransparent)
            {
                int VertexIndices[3] = {};
                int NormalIndices[3] = {};
                
                bool ParsedFace = CHParseFaceWithTexCoord(ObjFileWalker, VertexIndices, NormalIndices);
                CH_ASSERT(ParsedFace);
                
                for (int VI = 0; VI < 3; ++VI)
                {
                    //NOTE(chen): negative index wraps
                    if (VertexIndices[VI] < 0)
                    {
                        VertexIndices[VI] += (int)BufCount(TempVertices);
                    }
                    else
                    {
                        //NOTE(chen): OBJ is 1-based, making it 0-based
                        VertexIndices[VI] -= 1;
                    }
                    
                    //NOTE(chen): negative index wraps
                    if (NormalIndices[VI] < 0)
                    {
                        NormalIndices[VI] += (int)BufCount(TempNormals);
                    }
                    else
                    {
                        //NOTE(chen): OBJ is 1-based, making it 0-based
                        NormalIndices[VI] -= 1;
                    }
                    
                    CH_ASSERT(VertexIndices[VI] >= 0 && VertexIndices[VI] < BufCount(TempVertices));
                    CH_ASSERT(NormalIndices[VI] >= 0 && NormalIndices[VI] < BufCount(TempNormals));
                    
                    ch_vertex NewVertex = {};
                    
                    int VertexStartIndex = VertexIndices[VI] * 3;
                    memcpy(NewVertex.P, TempVertices + VertexStartIndex,
                           sizeof(float) * 3);
                    
                    int NormalStartIndex = NormalIndices[VI] * 3;
                    memcpy(NewVertex.N, TempNormals + NormalStartIndex,
                           sizeof(float) * 3);
                    
                    if (CurrentMatIndex != -1)
                    {
                        memcpy(NewVertex.Albedo, Mats[CurrentMatIndex].Albedo,
                               sizeof(float) * 3);
                        memcpy(NewVertex.Emission, Mats[CurrentMatIndex].Emission,
                               sizeof(float) * 3);
                    }
                    else
                    {
                        //NOTE(chen): default ch_material
                        NewVertex.Albedo[0] = 0.64f;
                        NewVertex.Albedo[1] = 0.64f;
                        NewVertex.Albedo[2] = 0.64f;
                        
                        NewVertex.Emission[0] = 0.0f;
                        NewVertex.Emission[1] = 0.0f;
                        NewVertex.Emission[2] = 0.0f;
                    }
                    
                    BufPush(Vertices, NewVertex);
                }
            }
            else
            {
                AlphaVertexCount += 3;
            }
        }
        else if (CHStartsWidth(ObjFileWalker, "usemtl"))
        {
            char MtlName[255];
            CHParseString(ObjFileWalker, "usemtl", MtlName);
            
            int NewMatIndex = -1;
            for (int MatIndex = 0; MatIndex < BufCount(Mats); ++MatIndex)
            {
                if (CHEqual(MtlName, Mats[MatIndex].Name))
                {
                    NewMatIndex = MatIndex;
                    break;
                }
            }
            
            CH_ASSERT(NewMatIndex != -1);
            CurrentMatIndex = NewMatIndex;
        }
        
        ObjFileWalker = CHGotoNextLine(ObjFileWalker);
    }
    free(ObjFileContent);
    
    BufFree(Mats);
    BufFree(TempVertices);
    BufFree(TempNormals);
    return Vertices;
}

#endif