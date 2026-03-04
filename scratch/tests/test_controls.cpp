#include "daisy_petal.h"
#include <cstdio>
#include <cmath>
#include <cassert>

using namespace daisy;

static void test_analog_control()
{
    uint16_t raw_val = 0;
    AnalogControl ctrl;
    ctrl.Init(&raw_val, 1000.f);

    raw_val = 0;
    for (int i = 0; i < 200; i++) ctrl.Process();
    assert(ctrl.Value() < 0.01f);

    raw_val = 65535;
    for (int i = 0; i < 200; i++) ctrl.Process();
    assert(ctrl.Value() > 0.95f);

    raw_val = 32768;
    for (int i = 0; i < 200; i++) ctrl.Process();
    float mid = ctrl.Value();
    assert(mid > 0.4f && mid < 0.6f);

    printf("  AnalogControl: PASS (min=~0, max=~1, mid=%.3f)\n", mid);
}

static void test_parameter_curves()
{
    uint16_t raw_val = 0;
    AnalogControl ctrl;
    ctrl.Init(&raw_val, 1000.f);

    Parameter pLin, pExp, pLog, pCube;
    pLin.Init(ctrl, 100.f, 1000.f, Parameter::LINEAR);
    pExp.Init(ctrl, 100.f, 1000.f, Parameter::EXPONENTIAL);
    pLog.Init(ctrl, 100.f, 1000.f, Parameter::LOGARITHMIC);
    pCube.Init(ctrl, 100.f, 1000.f, Parameter::CUBE);

    raw_val = 0;
    for (int i = 0; i < 500; i++) { pLin.Process(); pExp.Process(); pLog.Process(); pCube.Process(); }
    float lin0 = pLin.Value(), exp0 = pExp.Value(), log0 = pLog.Value(), cube0 = pCube.Value();

    raw_val = 32768;
    for (int i = 0; i < 500; i++) { pLin.Process(); pExp.Process(); pLog.Process(); pCube.Process(); }
    float linM = pLin.Value(), expM = pExp.Value(), logM = pLog.Value(), cubeM = pCube.Value();

    raw_val = 65535;
    for (int i = 0; i < 500; i++) { pLin.Process(); pExp.Process(); pLog.Process(); pCube.Process(); }
    float lin1 = pLin.Value(), exp1 = pExp.Value(), log1 = pLog.Value(), cube1 = pCube.Value();

    assert(lin0 < 200.f && lin1 > 900.f);
    assert(exp0 < 200.f && exp1 > 900.f);
    assert(log0 < 200.f && log1 > 800.f);
    assert(cube0 < 200.f && cube1 > 900.f);

    assert(expM < linM);
    assert(cubeM < linM);

    printf("  Parameter LINEAR:  min=%.0f mid=%.0f max=%.0f\n", lin0, linM, lin1);
    printf("  Parameter EXP:     min=%.0f mid=%.0f max=%.0f\n", exp0, expM, exp1);
    printf("  Parameter LOG:     min=%.0f mid=%.0f max=%.0f\n", log0, logM, log1);
    printf("  Parameter CUBE:    min=%.0f mid=%.0f max=%.0f\n", cube0, cubeM, cube1);
    printf("  Parameter curves: PASS\n");
}

static void test_switch()
{
    Switch sw;
    dsy_gpio_pin dummy = {DSY_GPIOA, 0};
    sw.Init(dummy, 0.f);

    assert(!sw.Pressed());
    assert(!sw.RisingEdge());
    assert(!sw.FallingEdge());

    sw.SetState(true);
    assert(sw.Pressed());
    assert(sw.RisingEdge());
    assert(!sw.FallingEdge());

    sw.SetState(true);
    assert(sw.Pressed());
    assert(!sw.RisingEdge());
    assert(!sw.FallingEdge());

    sw.SetState(false);
    assert(!sw.Pressed());
    assert(!sw.RisingEdge());
    assert(sw.FallingEdge());

    sw.SetState(false);
    assert(!sw.Pressed());
    assert(!sw.RisingEdge());
    assert(!sw.FallingEdge());

    printf("  Switch edges: PASS\n");
}

static void test_switch_via_petal()
{
    DaisyPetal hw;
    hw.Init();

    hw.switches[0].SetState(true);
    hw.ProcessDigitalControls();
    assert(hw.switches[0].Pressed());

    hw.switches[0].SetState(false);
    hw.ProcessDigitalControls();
    assert(!hw.switches[0].Pressed());

    printf("  Switch via DaisyPetal: PASS\n");
}

int main()
{
    printf("Milestone 2: AnalogControl + Switch tests\n");
    test_analog_control();
    test_parameter_curves();
    test_switch();
    test_switch_via_petal();
    printf("ALL PASS\n");
    return 0;
}
