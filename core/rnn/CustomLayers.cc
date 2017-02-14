/* ========================================================================== */
/*  Copyright (C) 2016 Hosang Yoon (hosangy@gmail.com) - All Rights Reserved  */
/*  Unauthorized copying of this file, via any medium is strictly prohibited  */
/*                        Proprietary and confidential                        */
/* ========================================================================== */

#include "CustomLayers.h"

#include <string>

namespace fractal
{

void AddLstmLayer_ForgetOneInit(Rnn &rnn,
                                const std::string &name,
                                const std::string &biasLayer,
                                const unsigned long delayAmount,
                                const unsigned long size,
                                const bool selfLoop,
                                const InitWeightParam &initWeightParam)
{
    const std::string prefix = name + ".";

    rnn.AddLayer(prefix + "INPUT", ACT_LINEAR, AGG_SUM, 4 * size);
    rnn.AddLayer(prefix + "INPUT_SQUASH", ACT_TANH, AGG_SUM, size);
    rnn.AddLayer(prefix + "INPUT_GATE_PEEP", ACT_LINEAR, AGG_MULT, size);
    rnn.AddLayer(prefix + "FORGET_GATE_PEEP", ACT_LINEAR, AGG_MULT, size);
    rnn.AddLayer(prefix + "OUTPUT_GATE_PEEP", ACT_LINEAR, AGG_MULT, size);
    rnn.AddLayer(prefix + "INPUT_GATE", ACT_SIGMOID, AGG_SUM, size);
    rnn.AddLayer(prefix + "INPUT_GATE_MULT", ACT_LINEAR, AGG_MULT, size);
    rnn.AddLayer(prefix + "MEMORY_CELL", ACT_LINEAR, AGG_SUM, size);
    rnn.AddLayer(prefix + "MEMORY_CELL.DELAYED", ACT_LINEAR, AGG_MULT, size);
    rnn.AddLayer(prefix + "FORGET_GATE", ACT_SIGMOID, AGG_SUM, size);
    rnn.AddLayer(prefix + "FORGET_GATE_MULT", ACT_LINEAR, AGG_MULT, size);
    rnn.AddLayer(prefix + "OUTPUT_SQUASH", ACT_TANH, AGG_SUM, size);
    rnn.AddLayer(prefix + "OUTPUT_GATE", ACT_SIGMOID, AGG_SUM, size);
    rnn.AddLayer(prefix + "OUTPUT", ACT_LINEAR, AGG_MULT, size);

    ConnParam connParam(CONN_IDENTITY);

    connParam.srcRangeFrom = 0;
    connParam.srcRangeTo = size - 1;
    rnn.AddConnection(prefix + "INPUT", prefix + "INPUT_SQUASH", connParam);

    connParam.srcRangeFrom += size;
    connParam.srcRangeTo += size;
    rnn.AddConnection(prefix + "INPUT", prefix + "INPUT_GATE", connParam);

    connParam.srcRangeFrom += size;
    connParam.srcRangeTo += size;
    rnn.AddConnection(prefix + "INPUT", prefix + "FORGET_GATE", connParam);

    connParam.srcRangeFrom += size;
    connParam.srcRangeTo += size;
    rnn.AddConnection(prefix + "INPUT", prefix + "OUTPUT_GATE", connParam);

    rnn.AddConnection(prefix + "INPUT_SQUASH", prefix + "INPUT_GATE_MULT", CONN_IDENTITY);
    rnn.AddConnection(prefix + "INPUT_GATE", prefix + "INPUT_GATE_MULT", CONN_IDENTITY);
    rnn.AddConnection(prefix + "INPUT_GATE_MULT", prefix + "MEMORY_CELL", CONN_IDENTITY);
    rnn.AddConnection(prefix + "MEMORY_CELL", prefix + "MEMORY_CELL.DELAYED", {CONN_IDENTITY, delayAmount});
    rnn.AddConnection(prefix + "MEMORY_CELL.DELAYED", prefix + "FORGET_GATE_MULT", CONN_IDENTITY);
    rnn.AddConnection(prefix + "FORGET_GATE", prefix + "FORGET_GATE_MULT", CONN_IDENTITY);
    rnn.AddConnection(prefix + "FORGET_GATE_MULT", prefix + "MEMORY_CELL", CONN_IDENTITY);
    rnn.AddConnection(prefix + "MEMORY_CELL", prefix + "OUTPUT_SQUASH", CONN_IDENTITY);
    rnn.AddConnection(prefix + "OUTPUT_SQUASH", prefix + "OUTPUT", CONN_IDENTITY);
    rnn.AddConnection(prefix + "OUTPUT_GATE", prefix + "OUTPUT", CONN_IDENTITY);

    /* Bias */
    ConnParam initParam(initWeightParam);

    initParam.dstRangeFrom = 0;
    initParam.dstRangeTo = size - 1;
    rnn.AddConnection(biasLayer, prefix + "INPUT", initParam);

    initParam.dstRangeFrom += size;
    initParam.dstRangeTo += size;
    rnn.AddConnection(biasLayer, prefix + "INPUT", initParam);

    InitWeightParamUniform oneInitParam;
    oneInitParam.a = 1.0;
    oneInitParam.b = 1.0;
    oneInitParam.isValid = true;

    ConnParam oneParam(oneInitParam);
    oneParam.dstRangeFrom = 2 * size;
    oneParam.dstRangeTo = 3 * size - 1;
    rnn.AddConnection(biasLayer, prefix + "INPUT", oneParam);

    initParam.dstRangeFrom += 2 * size;
    initParam.dstRangeTo += 2 * size;
    rnn.AddConnection(biasLayer, prefix + "INPUT", initParam);

    /* Peephole connections */
    rnn.AddConnection(prefix + "MEMORY_CELL.DELAYED", prefix + "INPUT_GATE_PEEP", CONN_IDENTITY);
    rnn.AddConnection(prefix + "MEMORY_CELL.DELAYED", prefix + "FORGET_GATE_PEEP", CONN_IDENTITY);
    rnn.AddConnection(prefix + "MEMORY_CELL", prefix + "OUTPUT_GATE_PEEP", CONN_IDENTITY);
    rnn.AddConnection(biasLayer, prefix + "INPUT_GATE_PEEP", initWeightParam);
    rnn.AddConnection(biasLayer, prefix + "FORGET_GATE_PEEP", initWeightParam);
    rnn.AddConnection(biasLayer, prefix + "OUTPUT_GATE_PEEP", initWeightParam);
    rnn.AddConnection(prefix + "INPUT_GATE_PEEP", prefix + "INPUT_GATE", CONN_IDENTITY);
    rnn.AddConnection(prefix + "FORGET_GATE_PEEP", prefix + "FORGET_GATE", CONN_IDENTITY);
    rnn.AddConnection(prefix + "OUTPUT_GATE_PEEP", prefix + "OUTPUT_GATE", CONN_IDENTITY);

    if(selfLoop == true)
    {
        rnn.AddLayer(prefix + "OUTPUT.DELAYED", ACT_LINEAR, AGG_MULT, size);

        rnn.AddConnection(prefix + "OUTPUT", prefix + "OUTPUT.DELAYED", {CONN_IDENTITY, delayAmount});
        rnn.AddConnection(prefix + "OUTPUT.DELAYED", prefix + "INPUT", initWeightParam);
    }
}

void AddResGateLayer(Rnn &rnn,
                     const std::string &name,
                     const std::string &biasLayer,
                     const unsigned long size)
{
    // out = in_r * sig(k) + in_1 * (1 - sig(k))
    // in_r : residual (output of layer just below)
    // in_1 : identity (input  of layer just below)
    // assumes in_r.size == in_1.size

    const std::string prefix = name + ".";

    rnn.AddLayer(prefix + "INPUT_R" , ACT_LINEAR          , AGG_MULT, size);
    rnn.AddLayer(prefix + "INPUT_1" , ACT_LINEAR          , AGG_MULT, size);
    rnn.AddLayer(prefix + "SWITCH_R", ACT_SIGMOID         , AGG_SUM , size);
    rnn.AddLayer(prefix + "SWITCH_1", ACT_ONE_MINUS_LINEAR, AGG_SUM , size);
    rnn.AddLayer(prefix + "OUTPUT"  , ACT_LINEAR          , AGG_SUM , size);
    
    InitWeightParamUniform minusOne; // biased towards identity initially
    minusOne.a = -1.0;
    minusOne.b = -1.0;
    minusOne.isValid = true;
    rnn.AddConnection(biasLayer          , prefix + "SWITCH_R", minusOne);
    rnn.AddConnection(prefix + "SWITCH_R", prefix + "SWITCH_1", CONN_IDENTITY);

    rnn.AddConnection(prefix + "SWITCH_R", prefix + "INPUT_R", CONN_IDENTITY);
    rnn.AddConnection(prefix + "SWITCH_1", prefix + "INPUT_1", CONN_IDENTITY);

    rnn.AddConnection(prefix + "INPUT_R", prefix + "OUTPUT", CONN_IDENTITY);
    rnn.AddConnection(prefix + "INPUT_1", prefix + "OUTPUT", CONN_IDENTITY);
}

void AddLstmLayer_LearnInit(Rnn &rnn,
                            const std::string &name,
                            const std::string &biasLayer,
                            const unsigned long delayAmount,
                            const unsigned long size,
                            const InitWeightParam &initWeightParam)
{
    const std::string prefix = name + ".";

    rnn.AddLayer(prefix + "INPUT", ACT_LINEAR, AGG_SUM, 4 * size);
    rnn.AddLayer(prefix + "INPUT_SQUASH", ACT_TANH, AGG_SUM, size);
    rnn.AddLayer(prefix + "INPUT_GATE_PEEP", ACT_LINEAR, AGG_MULT, size);
    rnn.AddLayer(prefix + "FORGET_GATE_PEEP", ACT_LINEAR, AGG_MULT, size);
    rnn.AddLayer(prefix + "OUTPUT_GATE_PEEP", ACT_LINEAR, AGG_MULT, size);
    rnn.AddLayer(prefix + "INPUT_GATE", ACT_SIGMOID, AGG_SUM, size);
    rnn.AddLayer(prefix + "INPUT_GATE_MULT", ACT_LINEAR, AGG_MULT, size);
    rnn.AddLayer(prefix + "MEMORY_CELL", ACT_LINEAR, AGG_SUM, size);
    rnn.AddLayer(prefix + "MEMORY_CELL.DELAYED", ACT_LINEAR, AGG_SUM, size); // mult -> sum
    rnn.AddLayer(prefix + "FORGET_GATE", ACT_SIGMOID, AGG_SUM, size);
    rnn.AddLayer(prefix + "FORGET_GATE_MULT", ACT_LINEAR, AGG_MULT, size);
    rnn.AddLayer(prefix + "OUTPUT_SQUASH", ACT_TANH, AGG_SUM, size);
    rnn.AddLayer(prefix + "OUTPUT_GATE", ACT_SIGMOID, AGG_SUM, size);
    rnn.AddLayer(prefix + "OUTPUT", ACT_LINEAR, AGG_MULT, size);
    rnn.AddLayer(prefix + "OUTPUT.DELAYED", ACT_LINEAR, AGG_SUM, size); // mult -> sum

    // switch between init & prev states
    // reset = 1. (use init states) or 0. (use prev states)
    {
        rnn.AddLayer(prefix + "RESET", ACT_LINEAR          , AGG_SUM, 1);
        rnn.AddLayer(prefix + "CARRY", ACT_ONE_MINUS_LINEAR, AGG_SUM, 1);
        rnn.AddConnection(prefix + "RESET", prefix + "CARRY", CONN_IDENTITY);

        rnn.AddLayer(prefix + "MEMORY_CELL_INIT", ACT_LINEAR, AGG_MULT, size);
        rnn.AddLayer(prefix + "MEMORY_CELL_PREV", ACT_LINEAR, AGG_MULT, size);
        rnn.AddLayer(prefix + "OUTPUT_INIT"     , ACT_LINEAR, AGG_MULT, size);
        rnn.AddLayer(prefix + "OUTPUT_PREV"     , ACT_LINEAR, AGG_MULT, size);

        rnn.AddConnection(biasLayer, prefix + "MEMORY_CELL_INIT", initWeightParam);
        rnn.AddConnection(biasLayer, prefix + "OUTPUT_INIT"     , initWeightParam);

        rnn.AddConnection(prefix + "RESET", prefix + "MEMORY_CELL_INIT", CONN_BROADCAST);
        rnn.AddConnection(prefix + "CARRY", prefix + "MEMORY_CELL_PREV", CONN_BROADCAST);
        rnn.AddConnection(prefix + "RESET", prefix + "OUTPUT_INIT"     , CONN_BROADCAST);
        rnn.AddConnection(prefix + "CARRY", prefix + "OUTPUT_PREV"     , CONN_BROADCAST);

        rnn.AddConnection(prefix + "MEMORY_CELL_INIT", prefix + "MEMORY_CELL.DELAYED", CONN_IDENTITY);
        rnn.AddConnection(prefix + "MEMORY_CELL_PREV", prefix + "MEMORY_CELL.DELAYED", CONN_IDENTITY);
        rnn.AddConnection(prefix + "OUTPUT_INIT"     , prefix + "OUTPUT.DELAYED"     , CONN_IDENTITY);
        rnn.AddConnection(prefix + "OUTPUT_PREV"     , prefix + "OUTPUT.DELAYED"     , CONN_IDENTITY);
    }

    ConnParam connParam(CONN_IDENTITY);

    connParam.srcRangeFrom = 0;
    connParam.srcRangeTo = size - 1;
    rnn.AddConnection(prefix + "INPUT", prefix + "INPUT_SQUASH", connParam);

    connParam.srcRangeFrom += size;
    connParam.srcRangeTo += size;
    rnn.AddConnection(prefix + "INPUT", prefix + "INPUT_GATE", connParam);

    connParam.srcRangeFrom += size;
    connParam.srcRangeTo += size;
    rnn.AddConnection(prefix + "INPUT", prefix + "FORGET_GATE", connParam);

    connParam.srcRangeFrom += size;
    connParam.srcRangeTo += size;
    rnn.AddConnection(prefix + "INPUT", prefix + "OUTPUT_GATE", connParam);

    rnn.AddConnection(prefix + "INPUT_SQUASH", prefix + "INPUT_GATE_MULT", CONN_IDENTITY);
    rnn.AddConnection(prefix + "INPUT_GATE", prefix + "INPUT_GATE_MULT", CONN_IDENTITY);
    rnn.AddConnection(prefix + "INPUT_GATE_MULT", prefix + "MEMORY_CELL", CONN_IDENTITY);
    rnn.AddConnection(prefix + "MEMORY_CELL", prefix + "MEMORY_CELL_PREV", {CONN_IDENTITY, delayAmount}); // .DELAYED -> _PREV
    rnn.AddConnection(prefix + "MEMORY_CELL.DELAYED", prefix + "FORGET_GATE_MULT", CONN_IDENTITY);
    rnn.AddConnection(prefix + "FORGET_GATE", prefix + "FORGET_GATE_MULT", CONN_IDENTITY);
    rnn.AddConnection(prefix + "FORGET_GATE_MULT", prefix + "MEMORY_CELL", CONN_IDENTITY);
    rnn.AddConnection(prefix + "MEMORY_CELL", prefix + "OUTPUT_SQUASH", CONN_IDENTITY);
    rnn.AddConnection(prefix + "OUTPUT_SQUASH", prefix + "OUTPUT", CONN_IDENTITY);
    rnn.AddConnection(prefix + "OUTPUT_GATE", prefix + "OUTPUT", CONN_IDENTITY);

    /* Bias */
    rnn.AddConnection(biasLayer, prefix + "INPUT", initWeightParam);

    /* Peephole connections */
    rnn.AddConnection(prefix + "MEMORY_CELL.DELAYED", prefix + "INPUT_GATE_PEEP", CONN_IDENTITY);
    rnn.AddConnection(prefix + "MEMORY_CELL.DELAYED", prefix + "FORGET_GATE_PEEP", CONN_IDENTITY);
    rnn.AddConnection(prefix + "MEMORY_CELL", prefix + "OUTPUT_GATE_PEEP", CONN_IDENTITY);
    rnn.AddConnection(biasLayer, prefix + "INPUT_GATE_PEEP", initWeightParam);
    rnn.AddConnection(biasLayer, prefix + "FORGET_GATE_PEEP", initWeightParam);
    rnn.AddConnection(biasLayer, prefix + "OUTPUT_GATE_PEEP", initWeightParam);
    rnn.AddConnection(prefix + "INPUT_GATE_PEEP", prefix + "INPUT_GATE", CONN_IDENTITY);
    rnn.AddConnection(prefix + "FORGET_GATE_PEEP", prefix + "FORGET_GATE", CONN_IDENTITY);
    rnn.AddConnection(prefix + "OUTPUT_GATE_PEEP", prefix + "OUTPUT_GATE", CONN_IDENTITY);

    // selfLoop
    {
        rnn.AddConnection(prefix + "OUTPUT", prefix + "OUTPUT_PREV", {CONN_IDENTITY, delayAmount}); // .DELAYED -> _PREV
        rnn.AddConnection(prefix + "OUTPUT.DELAYED", prefix + "INPUT", initWeightParam);
    }
}

void AddLstmLayer_DSigmoidOut(Rnn &rnn,
                              const std::string &name,
                              const std::string &biasLayer,
                              const unsigned long delayAmount,
                              const unsigned long size,
                              const bool selfLoop,
                              const InitWeightParam &initWeightParam)
{
    const std::string prefix = name + ".";

    rnn.AddLayer(prefix + "INPUT", ACT_LINEAR, AGG_SUM, 4 * size);
    rnn.AddLayer(prefix + "INPUT_SQUASH", ACT_TANH, AGG_SUM, size);
    rnn.AddLayer(prefix + "INPUT_GATE_PEEP", ACT_LINEAR, AGG_MULT, size);
    rnn.AddLayer(prefix + "FORGET_GATE_PEEP", ACT_LINEAR, AGG_MULT, size);
    rnn.AddLayer(prefix + "OUTPUT_GATE_PEEP", ACT_LINEAR, AGG_MULT, size);
    rnn.AddLayer(prefix + "INPUT_GATE", ACT_SIGMOID, AGG_SUM, size);
    rnn.AddLayer(prefix + "INPUT_GATE_MULT", ACT_LINEAR, AGG_MULT, size);
    rnn.AddLayer(prefix + "MEMORY_CELL", ACT_LINEAR, AGG_SUM, size);
    rnn.AddLayer(prefix + "MEMORY_CELL.DELAYED", ACT_LINEAR, AGG_MULT, size);
    rnn.AddLayer(prefix + "FORGET_GATE", ACT_SIGMOID, AGG_SUM, size);
    rnn.AddLayer(prefix + "FORGET_GATE_MULT", ACT_LINEAR, AGG_MULT, size);
    rnn.AddLayer(prefix + "OUTPUT_SIGMOID", ACT_SIGMOID, AGG_SUM, size);
    rnn.AddLayer(prefix + "OUTPUT_ONE_MINUS_SIGMOID", ACT_ONE_MINUS_LINEAR, AGG_SUM, size);
    rnn.AddLayer(prefix + "OUTPUT_DSIGMOID", ACT_LINEAR, AGG_MULT, size);
    rnn.AddLayer(prefix + "OUTPUT_GATE", ACT_SIGMOID, AGG_SUM, size);
    rnn.AddLayer(prefix + "OUTPUT", ACT_LINEAR, AGG_MULT, size);

    ConnParam connParam(CONN_IDENTITY);

    connParam.srcRangeFrom = 0;
    connParam.srcRangeTo = size - 1;
    rnn.AddConnection(prefix + "INPUT", prefix + "INPUT_SQUASH", connParam);

    connParam.srcRangeFrom += size;
    connParam.srcRangeTo += size;
    rnn.AddConnection(prefix + "INPUT", prefix + "INPUT_GATE", connParam);

    connParam.srcRangeFrom += size;
    connParam.srcRangeTo += size;
    rnn.AddConnection(prefix + "INPUT", prefix + "FORGET_GATE", connParam);

    connParam.srcRangeFrom += size;
    connParam.srcRangeTo += size;
    rnn.AddConnection(prefix + "INPUT", prefix + "OUTPUT_GATE", connParam);

    rnn.AddConnection(prefix + "INPUT_SQUASH", prefix + "INPUT_GATE_MULT", CONN_IDENTITY);
    rnn.AddConnection(prefix + "INPUT_GATE", prefix + "INPUT_GATE_MULT", CONN_IDENTITY);
    rnn.AddConnection(prefix + "INPUT_GATE_MULT", prefix + "MEMORY_CELL", CONN_IDENTITY);
    rnn.AddConnection(prefix + "MEMORY_CELL", prefix + "MEMORY_CELL.DELAYED", {CONN_IDENTITY, delayAmount});
    rnn.AddConnection(prefix + "MEMORY_CELL.DELAYED", prefix + "FORGET_GATE_MULT", CONN_IDENTITY);
    rnn.AddConnection(prefix + "FORGET_GATE", prefix + "FORGET_GATE_MULT", CONN_IDENTITY);
    rnn.AddConnection(prefix + "FORGET_GATE_MULT", prefix + "MEMORY_CELL", CONN_IDENTITY);
    rnn.AddConnection(prefix + "MEMORY_CELL", prefix + "OUTPUT_SIGMOID", CONN_IDENTITY);
    rnn.AddConnection(prefix + "OUTPUT_SIGMOID", prefix + "OUTPUT_ONE_MINUS_SIGMOID", CONN_IDENTITY);
    rnn.AddConnection(prefix + "OUTPUT_SIGMOID", prefix + "OUTPUT_DSIGMOID", CONN_IDENTITY);
    rnn.AddConnection(prefix + "OUTPUT_ONE_MINUS_SIGMOID", prefix + "OUTPUT_DSIGMOID", CONN_IDENTITY);
    rnn.AddConnection(prefix + "OUTPUT_DSIGMOID", prefix + "OUTPUT", CONN_IDENTITY);
    rnn.AddConnection(prefix + "OUTPUT_GATE", prefix + "OUTPUT", CONN_IDENTITY);

    /* Bias */
    rnn.AddConnection(biasLayer, prefix + "INPUT", initWeightParam);

    /* Peephole connections */
    rnn.AddConnection(prefix + "MEMORY_CELL.DELAYED", prefix + "INPUT_GATE_PEEP", CONN_IDENTITY);
    rnn.AddConnection(prefix + "MEMORY_CELL.DELAYED", prefix + "FORGET_GATE_PEEP", CONN_IDENTITY);
    rnn.AddConnection(prefix + "MEMORY_CELL", prefix + "OUTPUT_GATE_PEEP", CONN_IDENTITY);
    rnn.AddConnection(biasLayer, prefix + "INPUT_GATE_PEEP", initWeightParam);
    rnn.AddConnection(biasLayer, prefix + "FORGET_GATE_PEEP", initWeightParam);
    rnn.AddConnection(biasLayer, prefix + "OUTPUT_GATE_PEEP", initWeightParam);
    rnn.AddConnection(prefix + "INPUT_GATE_PEEP", prefix + "INPUT_GATE", CONN_IDENTITY);
    rnn.AddConnection(prefix + "FORGET_GATE_PEEP", prefix + "FORGET_GATE", CONN_IDENTITY);
    rnn.AddConnection(prefix + "OUTPUT_GATE_PEEP", prefix + "OUTPUT_GATE", CONN_IDENTITY);

    if(selfLoop == true)
    {
        rnn.AddLayer(prefix + "OUTPUT.DELAYED", ACT_LINEAR, AGG_MULT, size);

        rnn.AddConnection(prefix + "OUTPUT", prefix + "OUTPUT.DELAYED", {CONN_IDENTITY, delayAmount});
        rnn.AddConnection(prefix + "OUTPUT.DELAYED", prefix + "INPUT", initWeightParam);
    }
}

void AddOELstmLayer(Rnn &rnn,
                    const std::string &name,
                    const std::string &biasLayer,
                    const unsigned long delayAmount,
                    const unsigned long size,
                    const bool selfLoop,
                    const InitWeightParam &initWeightParam)
{
    const std::string prefix = name + ".";
    
    verify(size % 2 == 0);

    rnn.AddLayer(prefix + "INPUT", ACT_LINEAR, AGG_SUM, 4 * size);

    // 1D weights (input bias & peephole) are added by the internal layers
    // 2D weights (below-to-input & feedback) are added outside
    basicLayers::AddFastLstmLayer(rnn, prefix + "OLSTM", biasLayer, 1,
                                  size / 2, false, initWeightParam);
    AddLstmLayer_DSigmoidOut     (rnn, prefix + "ELSTM", biasLayer, 1,
                                  size / 2, false, initWeightParam);
    
    /* Connect "RESET" to
     * "OLSTM.MEMORY_CELL.DELAYED" and "ELSTM.MEMORY_CELL.DELAYED" directly
     *
     * Otherwise, cannot function with "RESET" removed for inference
     */

    rnn.AddLayer(prefix + "OUTPUT", ACT_LINEAR, AGG_SUM, size);

    ConnParam srcParam(CONN_IDENTITY);

    srcParam.srcRangeFrom = 0;
    srcParam.srcRangeTo   = 2 * size - 1;
    rnn.AddConnection(prefix + "INPUT", prefix + "OLSTM.INPUT", srcParam);

    srcParam.srcRangeFrom += 2 * size;
    srcParam.srcRangeTo   += 2 * size;
    rnn.AddConnection(prefix + "INPUT", prefix + "ELSTM.INPUT", srcParam);

    // // relay reset signal
    // rnn.AddLayer(prefix + "MEMORY_CELL.DELAYED", ACT_LINEAR, AGG_SUM, size);
    // 
    // srcParam.srcRangeFrom = 0;
    // srcParam.srcRangeTo   = size / 2 - 1;
    // rnn.AddConnection(prefix + "MEMORY_CELL.DELAYED",
    //                   prefix + "OLSTM.MEMORY_CELL.DELAYED", srcParam);
    // 
    // srcParam.srcRangeFrom += size / 2;
    // srcParam.srcRangeTo   += size / 2;
    // rnn.AddConnection(prefix + "MEMORY_CELL.DELAYED",
    //                   prefix + "ELSTM.MEMORY_CELL.DELAYED", srcParam);

    ConnParam dstParam(CONN_IDENTITY);

    dstParam.dstRangeFrom = 0;
    dstParam.dstRangeTo   = size / 2 - 1;
    rnn.AddConnection(prefix + "OLSTM.OUTPUT", prefix + "OUTPUT", dstParam);

    dstParam.dstRangeFrom += size / 2;
    dstParam.dstRangeTo   += size / 2;
    rnn.AddConnection(prefix + "ELSTM.OUTPUT", prefix + "OUTPUT", dstParam);

    if(selfLoop == true)
    {
        rnn.AddLayer(prefix + "OUTPUT.DELAYED", ACT_LINEAR, AGG_MULT, size);

        rnn.AddConnection(prefix + "OUTPUT", prefix + "OUTPUT.DELAYED", {CONN_IDENTITY, delayAmount});
        rnn.AddConnection(prefix + "OUTPUT.DELAYED", prefix + "INPUT", initWeightParam);
    }
}

}