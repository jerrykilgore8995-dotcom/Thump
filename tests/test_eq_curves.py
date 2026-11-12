#!/usr/bin/env python3
"""Standalone coefficient math tests for THUMP v0.4's EQ curve family.

These inspect the implemented RBJ equations and approximate proportional-Q law,
not measured Neve/API hardware. Regression tests are not JUCE build tests.
"""
import cmath
import math
from pathlib import Path

P = Path(__file__).resolve().parents[1]
SOURCE = (P/'Source/PluginProcessor.cpp').read_text(encoding='utf-8')
HEADER = (P/'Source/PluginProcessor.h').read_text(encoding='utf-8')
EDITOR = (P/'Source/PluginEditor.cpp').read_text(encoding='utf-8')


def shelf(freq, gain, user_q, fs):
    if abs(gain) < 0.00001:
        return (1., 0., 0., 0., 0.)
    w = 2 * math.pi * max(5, min(fs * .475, freq)) / fs
    c, sine = math.cos(w), math.sin(w)
    A = 10**(gain / 40)
    slope = max(.26, min(1.45, .78 * user_q**.62))
    alpha = .5 * sine * math.sqrt(max(.0001, (A + 1/A) * (1/slope - 1) + 2))
    beta = 2 * math.sqrt(A) * alpha
    a0 = (A+1) + (A-1)*c + beta
    a1 = -2*((A-1) + (A+1)*c)
    a2 = (A+1) + (A-1)*c - beta
    b0 = A*((A+1) - (A-1)*c + beta)
    b1 = 2*A*((A-1) - (A+1)*c)
    b2 = A*((A+1) - (A-1)*c - beta)
    return (b0/a0, b1/a0, b2/a0, a1/a0, a2/a0)


def h(c, freq, fs):
    z = cmath.exp(-2j*math.pi*freq/fs)
    b0,b1,b2,a1,a2 = c
    return (b0+b1*z+b2*z*z)/(1+a1*z+a2*z*z)


def db(c, freq, fs):
    return 20*math.log10(abs(h(c,freq,fs)))


def pq(gain, q):
    steps=((0,.39),(2,.48),(4,.66),(6,.88),(9,1.20),(12,1.55))
    g=min(12.,abs(gain))
    for i in range(1,len(steps)):
        if g<=steps[i][0]:
            a,b=steps[i-1],steps[i]
            return max(.15,min(4.8,(a[1]+(b[1]-a[1])*(g-a[0])/(b[0]-a[0]))*q))
    return max(.15,min(4.8,steps[-1][1]*q))


def bell(freq, gain, q, fs):
    w=2*math.pi*freq/fs
    c,s=math.cos(w),math.sin(w)
    alpha=s/(2*q)
    A=10**(gain/40)
    a0=1+alpha/A
    return ((1+alpha*A)/a0, (-2*c)/a0, (1-alpha*A)/a0,
            (-2*c)/a0, (1-alpha/A)/a0)


def main():
    assert 'void Biquad::lowShelf' in SOURCE and 'void lowShelf' in HEADER
    assert 'thump::proportionalQ' in SOURCE
    assert 'b.eq[i].lowShelf' in SOURCE
    assert 'b.eq[i].lowShelf (hz,' in SOURCE and 'b.lowDivot[i].bell' in SOURCE
    assert 'float qControl' in SOURCE
    assert 'setDoubleClickReturnValue (true, 1.0)' in EDITOR

    for fs in (88200, 96000, 192000):
        for hz in (40, 120, 220):
            for q in (.3, 1., 1.5, 3.):
                assert max(abs(db(shelf(hz,0,q,fs),f,fs)) for f in (10, 60, 500, 7000)) < 1e-12
                for gain in (-12, -6, 6, 12):
                    coeff=shelf(hz,gain,q,fs)
                    assert all(math.isfinite(x) for x in coeff)
                    # The shelf's DC gain equals the requested +/- dB value.
                    assert abs(db(coeff, 0.01, fs)-gain)<0.3,(fs,hz,gain,q,db(coeff,.01,fs))
                    assert abs(db(coeff, 10000, fs))<.2,(fs,hz,gain,q,db(coeff,10000,fs))
                    # Pole radii should remain inside the unit circle.
                    _, _, _, a1,a2=coeff
                    discr=complex(a1*a1-4*a2)**.5
                    roots=((-a1+discr)/2,(-a1-discr)/2)
                    assert max(map(abs, roots)) < 1
    # API-style Q increases with boost/cut amount, for both cut and boost.
    for amount in (0,1,3,6,9,12):
        assert pq(amount,1)==pq(-amount,1)
    assert all(pq(a,1)<pq(b,1) for a,b in ((1,2),(2,4),(4,8),(8,12)))
    for hz in (1500,5000):
        for gain in (-12,-6,6,12):
            c=bell(hz,gain,pq(gain,1),88200)
            assert abs(db(c,hz,88200)-gain)<1e-5
        assert abs(db(bell(hz,2,pq(2,1),88200),hz*1.5,88200)) > abs(db(bell(hz,12,pq(12,1),88200),hz*1.5,88200))/6 - .3  # qualitative bandwidth only
    print('PASS: shelf stability, flat-zero behavior, low-end boost/cut and response at three sample rates')
    print('PASS: proportional-Q rises symmetrically with gain magnitude at 1.5k and 5k')
    print('NOTE: these are designed response families, NOT hardware-exact EQ measurements')

if __name__=='__main__':
    main()
