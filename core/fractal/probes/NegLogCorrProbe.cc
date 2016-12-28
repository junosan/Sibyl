/*
   Copyright 2015 Kyuyeon Hwang (kyuyeon.hwang@gmail.com)

   Licensed under the Apache License, Version 2.0 (the "License");
   you may not use this file except in compliance with the License.
   You may obtain a copy of the License at

       http://www.apache.org/licenses/LICENSE-2.0

   Unless required by applicable law or agreed to in writing, software
   distributed under the License is distributed on an "AS IS" BASIS,
   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
   See the License for the specific language governing permissions and
   limitations under the License.
*/


#include "NegLogCorrProbe.h"

#include <cmath>

#include "../core/Layer.h"


namespace fractal
{


NegLogCorrProbe::NegLogCorrProbe() : TrainableProbe(true)
{
    ResetStatistics();
}


void NegLogCorrProbe::SetTarget(MultiTypeMatrix &mat, const unsigned long idxFrom, const unsigned long idxTo)
{
    unsigned long nRows, nCols;

    verify(engine != NULL);

    nRows = linkedLayer->GetSize();
    nCols = nStream * nUnroll;

    target.Resize(nRows, nCols);
    temp.Resize(nRows, nCols);


    Matrix<FLOAT> targetSub(target, idxFrom * nStream, idxTo * nStream + nStream - 1);

    switch(mat.GetDataType())
    {
        case MultiTypeMatrix::DATATYPE_FLOAT:
            {
                Matrix<FLOAT> *ptrMat = reinterpret_cast<Matrix<FLOAT> *>(mat.GetMatrix());

                verify(ptrMat->GetNumRows() == nRows);
                verify(ptrMat->GetNumCols() == nStream * (idxTo - idxFrom + 1));

                engine->MatCopy(*ptrMat, targetSub, stream);
            }
            break;

        case MultiTypeMatrix::DATATYPE_INT:
            {
                Matrix<INT> *ptrMat = reinterpret_cast<Matrix<INT> *>(mat.GetMatrix());

                verify(ptrMat->GetNumRows() == 1);
                verify(ptrMat->GetNumCols() == nStream * (idxTo - idxFrom + 1));

                engine->OneHotEncode(*ptrMat, targetSub, stream);
            }
            break;

        default:
            verify(false);
    }
}


void NegLogCorrProbe::ComputeErr(const unsigned long idxFrom, const unsigned long idxTo)
{
    verify(engine != NULL);

    Matrix<FLOAT> targetSub(target, idxFrom * nStream, idxTo * nStream + nStream - 1);
    Matrix<FLOAT> errSub(err, idxFrom * nStream, idxTo * nStream + nStream - 1);
    Matrix<FLOAT> actSub(GetActivation(), idxFrom * nStream, idxTo * nStream + nStream - 1);
    Matrix<FLOAT> tempSub(temp, idxFrom * nStream, idxTo * nStream + nStream - 1);

    /* err = target .* (1 - act) */
    engine->MatCopy(targetSub, errSub, stream);
    engine->MatElemMult(targetSub, actSub, tempSub, stream);
    engine->MatAdd(tempSub, errSub, (FLOAT) -1, stream);
}


void NegLogCorrProbe::EvaluateOnHost(MultiTypeMatrix &target, Matrix<FLOAT> &output)
{
    double nlcPartialSum;
    unsigned long n, dim, nFrame;

    dim = output.GetNumRows();
    nFrame = output.GetNumCols();

    nlcPartialSum = 0.0;

    nSample += nFrame;

    switch(target.GetDataType())
    {
        case MultiTypeMatrix::DATATYPE_FLOAT:
            {
                Matrix<FLOAT> *targetMat = reinterpret_cast<Matrix<FLOAT> *>(target.GetMatrix());
                FLOAT *t, *o;

                verify(dim == targetMat->GetNumRows());
                verify(nFrame == targetMat->GetNumCols());

                PStream hostStream;

                engine->StreamCreate(hostStream, engine->GetHostLoc());

                targetMat->HostPull(hostStream);
                output.HostPull(hostStream);

                engine->StreamSynchronize(hostStream);
                engine->StreamDestroy(hostStream);

                t = targetMat->GetHostData();
                o = output.GetHostData();

                n = dim * nFrame;


                #ifdef FRACTAL_USE_OMP
                #pragma omp parallel for reduction(+:nlcPartialSum)
                #endif
                for(unsigned long i = 0; i < n; i++)
                {
                    FLOAT err = -t[i] * std::log(o[i] + (double) 1e-300);
                    nlcPartialSum += err;
                }
            }
            break;

        case MultiTypeMatrix::DATATYPE_INT:
            {
                Matrix<INT> *targetMat = reinterpret_cast<Matrix<INT> *>(target.GetMatrix());
                FLOAT *o;
                INT *t;

                verify(targetMat->GetNumRows() == 1);
                verify(nFrame == targetMat->GetNumCols());

                PStream hostStream;

                engine->StreamCreate(hostStream, engine->GetHostLoc());

                targetMat->HostPull(hostStream);
                output.HostPull(hostStream);

                engine->StreamSynchronize(hostStream);
                engine->StreamDestroy(hostStream);

                t = targetMat->GetHostData();
                o = output.GetHostData();


                #ifdef FRACTAL_USE_OMP
                #pragma omp parallel for reduction(+:nlcPartialSum)
                #endif
                for(unsigned long i = 0; i < nFrame; i++)
                {
                    for(unsigned long j = 0; j < dim; j++)
                    {
                        unsigned long idx = i * dim + j;

                        FLOAT err = -(FLOAT)((INT) j == t[i]) * std::log(o[idx] + (double) 1e-300);
                        nlcPartialSum += err;
                    }
                }
            }
            break;

        default:
            verify(false);
    }

    nlcSum += nlcPartialSum;
}


void NegLogCorrProbe::ResetStatistics()
{
    nSample = 0;
    nlcSum = 0.0;
}


const double NegLogCorrProbe::GetLoss()
{
    return GetNegLogCorr();
}


void NegLogCorrProbe::PrintStatistics(std::ostream &outStream)
{
    outStream << "NegLogCorr: " << GetNegLogCorr();
}


const double NegLogCorrProbe::GetNegLogCorr()
{
    return nlcSum / nSample;
}


void NegLogCorrProbe::SetEngine(Engine *engine)
{
    TrainableProbe::SetEngine(engine);

    target.SetEngine(engine);
    temp.SetEngine(engine);
}


}

