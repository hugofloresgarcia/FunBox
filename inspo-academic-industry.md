# Academic Research Done in Concert with Guitar Pedal / Audio Effects Companies

An index of PhD theses, master's theses, and key publications produced in direct
collaboration with—or with strong ties to—guitar effects and audio hardware companies.

---

## Theses with Explicit Industry Collaboration

### Etienne Thuillier — *Real-Time Polyphonic Octave Doubling for the Guitar* (2016)
- **Degree:** M.Sc., Aalto University
- **Supervisor:** Prof. Vesa Välimäki
- **Industry partner:** TC Electronic (Aarhus, Denmark)
- **Details:** Thuillier worked with TC Electronic's guitar effect team during April–June 2014.
  Evaluated multiple polyphonic octave-doubling methods and proposed two novel algorithms:
  ERB-PS2 (filter-bank phase scaling) and ERB-SSM2.
  ERB-PS2 achieved the best overall performance with greatly reduced latency vs. commercial pedals.
  This is the algorithm behind the **TC Electronic Sub'N'Up**.
- **PDF:** <https://aaltodoc.aalto.fi/server/api/core/bitstreams/ff9e52cf-fd79-45eb-b695-93038244ec0e/content>
- **Open-source implementation:** [terrarium-poly-octave](https://forum.pedalpcb.com/threads/terrarium-polyphonic-octaves-code-demo.23453/) by Steve Schulteis (Terrarium / PedalPCB platform)

### Alec Wright — *Neural Modelling of Audio Effects* (2023)
- **Degree:** D.Sc. (Tech.), Aalto University
- **Supervisor:** Prof. Vesa Välimäki
- **Industry partner:** Neural DSP Technologies (Helsinki, Finland)
- **Details:** Neural network methods for emulating analog guitar amplifiers, distortion pedals,
  compressors, and time-varying effects. Models run in real-time with low latency.
  Key co-authors from Neural DSP include Eero-Pekka Damskägg.
  Listening tests showed results difficult to distinguish from real devices.
- **Link:** <https://aaltodoc.aalto.fi/items/f376f16e-982a-485e-8412-1cb8362f9908>
- **Code:** <https://github.com/Alec-Wright/CoreAudioML>

### Jaromír Mačák — *Real-Time Digital Simulation of Guitar Amplifiers as Audio Effects* (2012)
- **Degree:** Ph.D., Brno University of Technology (VUT)
- **Supervisor:** Ing. Jiří Schimmel, Ph.D.
- **Industry partner:** Audiffex (Czech Republic)
- **Details:** Algorithms for simulating nonlinear analog guitar amplifier circuits in real-time.
  Used the DK-method for circuit analysis and nonlinear wave digital filters.
  Mačák thanks "Audiffex company for the opportunity to implement algorithms for the
  simulation of guitar analog effects in their products."
- **PDF:** <https://ccrma.stanford.edu/~jingjiez/portfolio/gtr-amp-sim/pdfs/Real-time%20Digital%20Simulation%20of%20Guitar%20Amplifiers%20as%20Audio%20Effects.pdf>

---

## Publications with Direct Industry Authorship

### Neural DSP + Aalto University — *End-to-End Amp Modeling* (2024)
- **Authors:** Lauri Juvela (Aalto), Eero-Pekka Damskägg, Aleksi Peussa, Jaakko Mäkinen,
  Thomas Sherson, Stylianos I. Mimilakis, Athanasios Gotsopoulos (Neural DSP)
- **Venue:** IEEE ICASSP 2023; arXiv 2024
- **Details:** Data-driven LSTM approach for controllable guitar amplifier models.
  Non-intrusive automated data collection. Performance comparable to SPICE circuit simulations.
  This is the technology behind **Neural DSP / Quad Cortex** amp captures.
- **Link:** <https://arxiv.org/abs/2403.08559>

### Neural DSP + Aalto — *Real-Time Guitar Amplifier Emulation with Deep Learning* (2020)
- **Authors:** Alec Wright, Eero-Pekka Damskägg (Neural DSP), Lauri Juvela, Vesa Välimäki
- **Venue:** Applied Sciences 10(3), 766
- **Details:** Compared WaveNet-based feedforward and RNN models for black-box amp/pedal modeling.
  Tested on Ibanez Tube Screamer, Boss DS-1, EHX Big Muff Pi, Blackstar HT-5 Metal,
  Mesa Boogie 5:50 Plus. Three minutes of audio data sufficed for training.
- **PDF:** <https://www.mdpi.com/2076-3417/10/3/766>

### Neural DSP + Aalto — *Real-Time Modeling of Audio Distortion Circuits with Deep Learning* (2019)
- **Authors:** Eero-Pekka Damskägg (Neural DSP), Lauri Juvela, Vesa Välimäki
- **Venue:** DAFx-2019
- **Details:** WaveNet architecture for black-box modeling of distortion pedals (Tube Screamer,
  DS-1, Big Muff Pi). Established the viability of neural amp modeling for real-time use.
- **Link:** <https://research.aalto.fi/en/publications/real-time-modeling-of-audio-distortion-circuits-with-deep-learnin>

### Jonathan Abel — CCRMA / Universal Audio
- **Role:** Co-Founder & CTO of Universal Audio; Consulting Professor at Stanford CCRMA
- **Details:** Extensive publications on audio effects modeling: physical modeling of reverb,
  dynamic convolution, allpass filter design, spring reverb emulation.
  His dual role bridges Stanford's academic research directly into UA's plugin products.
- **Publications:** <https://ccrma.stanford.edu/papers/author/1348>
- **DAFx papers:** <https://dafx.de/paper-archive/search?author[]=Abel,%20J.%20S.>

### Jean Laroche & Mark Dolson — *New Phase-Vocoder Techniques for Pitch-Shifting* (1999)
- **Affiliation:** Joint E-mu/Creative Technology Center
- **Venue:** IEEE WASPAA 1999
- **Details:** Introduced peak-based phase vocoder for direct frequency-domain pitch shifting,
  harmonizing, and spectral manipulation. Foundation for modern polyphonic pitch effects
  in Eventide, DigiTech, and other products.
- **PDF:** <https://www.ee.columbia.edu/~dpwe/papers/LaroD99-pvoc.pdf>

### Jon Dattorro — *Effect Design* Parts 1–3 (1997–2002)
- **Affiliation:** Stanford CCRMA (previously worked at Lexicon on reverb algorithm design)
- **Venue:** J. Audio Eng. Soc.
- **Details:** Canonical tutorial on reverb, chorus, delay, and oscillator algorithm design.
  The "Dattorro plate reverb" described in Part 1 is one of the most widely implemented
  reverb topologies in both hardware and software effects.
- **Part 1 (Reverb):** <https://ccrma.stanford.edu/~dattorro/EffectDesignPart1.pdf>
- **Part 2 (Chorus/Delay):** <https://ccrma.stanford.edu/~dattorro/EffectDesignPart2.pdf>
- **Part 3 (Oscillators):** <https://ccrma.stanford.edu/~dattorro/EffectDesignPart3.pdf>

---

## Doctoral Theses in Guitar Effects Modeling (Academic, Closely Related to Industry)

### David T. Yeh — *Digital Implementation of Musical Distortion Circuits by Analysis and Simulation* (2009)
- **Degree:** Ph.D., Stanford University (CCRMA)
- **Advisors:** Julius O. Smith III
- **Details:** Systematic methods for deriving nonlinear digital filters from analog circuit
  schematics (the "K-method"). Modeled guitar amplifiers, the Fender Bassman tone stack,
  and distortion pedal circuits. Foundational work for the virtual analog modeling field.
- **PDF:** <https://ccrma.stanford.edu/~dtyeh/papers/DavidYehThesisdoublesided.pdf>
- **Publications:** <https://ccrma.stanford.edu/~dtyeh/papers/pubs.html>

### Kurt James Werner — *Virtual Analog Modeling of Audio Circuitry Using Wave Digital Filters* (2016)
- **Degree:** Ph.D., Stanford University (CCRMA)
- **Advisors:** Julius O. Smith III, Jonathan Abel
- **Details:** Extended wave digital filter (WDF) theory to handle circuits with complex
  topologies and multiple nonlinearities (diodes, transistors, triodes). Case studies on
  the Roland TR-808 bass drum circuit. Enables systematic simulation of a vastly larger
  class of audio circuits.
- **PDF:** <https://stacks.stanford.edu/file/druid:jy057cz8322/KurtJamesWernerDissertation-augmented.pdf>

### Fabián Esqueda — *Aliasing Reduction in Nonlinear Audio Signal Processing* (2018)
- **Degree:** D.Sc. (Tech.), Aalto University
- **Supervisor:** Prof. Vesa Välimäki
- **Details:** Developed antiderivative antialiasing methods for nonlinear waveshaping
  (distortion, clipping). Collaborated with Stefan Bilbao (Edinburgh) and Julian D. Parker.
  Directly applicable to digital guitar distortion pedals.
- **PDF:** <https://aaltodoc.aalto.fi/bitstreams/e0b34035-b20e-457e-85c3-e42c4951363f/download>

### Ben Holmes — *Guitar Effects-Pedal Emulation and Identification* (2019)
- **Degree:** Ph.D., Queen's University Belfast
- **Supervisors:** Maarten van Walstijn, Stuart Ferguson
- **Details:** Physical modeling of analog guitar pedals using circuit component models,
  without destructive deconstruction. Developed identification procedures to optimize
  component parameters from input/output measurements only.
  Case study: Dallas Rangemaster Treble Booster.
- **Link:** <https://pure.qub.ac.uk/en/studentTheses/guitar-effects-pedal-emulation-and-identification/>

---

## Master's Theses

### Tantep Sinjanakhom — *Neural Modeling of Guitar Tone Stacks* (2024)
- **Degree:** M.Sc., Aalto University
- **Advisor:** Eero-Pekka Damskägg (Neural DSP)
- **Details:** Neural networks with differentiable filters for modeling 10 guitar amplifier
  tone stack circuits. Real-time implementation for audio plugins.
- **Link:** <https://aaltodoc.aalto.fi/items/a3ec2d35-8656-4e62-8714-36b702846c66>

### Antoni Jankowski — *Polyphonic Pitch Recognition for Guitar with Neural Networks* (2025)
- **Degree:** M.Sc., Aalto University
- **Details:** Neural network approach to polyphonic guitar pitch detection.
- **Listed at:** <https://aalto.fi/en/aalto-acoustics-lab/masters-theses-in-acoustics-and-audio-technology>

### Théo Royer — *Pitch Shifting Algorithms and Their Applications in Music* (2019)
- **Degree:** M.Sc., KTH Royal Institute of Technology
- **Details:** Evaluated PSOLA and phase vocoder approaches for pitch shifting.
  Combined harmonic-percussive separation with spectral envelope estimation.
  Quality comparable to commercial algorithms.
- **PDF:** <https://kth.diva-portal.org/smash/get/diva2:1381398/FULLTEXT01.pdf>

---

## White Papers & Industry Technical Publications

### Strymon — Engineering White Papers
- **Author:** Pete Celi (Lead DSP Engineer)
- **Topics:** Spring/plate/hall reverb modeling, digital delay (ADM / PCM / 24-96),
  rotary speaker Doppler simulation, amplifier tremolo circuits, tape dynamics.
- **Link:** <https://www.strymon.net/category/white-papers/>

### Jatin Chowdhury — *A Comparison of Virtual Analog Modelling Techniques* (2020)
- **Affiliation:** Stanford CCRMA / Chowdhury DSP
- **Details:** Virtual analog model of the **Klon Centaur** guitar pedal comparing nodal analysis,
  wave digital filters, and recurrent neural networks for both desktop plugins and
  embedded (guitar pedal) implementations.
- **PDF:** <https://ccrma.stanford.edu/~jatin/papers/Klon_Model.pdf>
- **Code:** <https://github.com/jatinchowdhury18/KlonCentaur>

---

## Key Research Groups & Labs

| Lab | University | PI | Industry Connections |
|-----|-----------|-----|---------------------|
| Audio Signal Processing | Aalto University | Vesa Välimäki | Neural DSP, TC Electronic, Genelec |
| CCRMA | Stanford University | Julius O. Smith III, Jonathan Abel | Universal Audio, Eventide, Line 6 |
| Acoustics & Audio Group | University of Edinburgh | Stefan Bilbao | Physical modeling community |
| Signal Processing | Helmut Schmidt University, Hamburg | Udo Zölzer | DAFx conference founder |
| SARC | Queen's University Belfast | Maarten van Walstijn | Virtual analog modeling |
| Dept. of Telecommunications | Brno University of Technology | Jiří Schimmel | Audiffex |

---

## Key Conferences & Venues

- **DAFx** (Digital Audio Effects) — <https://dafx.de> — The primary venue for guitar effects
  DSP research. Paper archive searchable at <https://dafx.de/paper-archive/>
- **IEEE ICASSP** — Major signal processing conference; Neural DSP has presented here
- **AES** (Audio Engineering Society) — Convention papers and journal articles
- **IEEE WASPAA** — Workshop on Applications of Signal Processing to Audio and Acoustics
- **SMC** (Sound and Music Computing) — European conference covering audio DSP
- **ADC** (Audio Developer Conference) — Industry-focused, often features pedal/plugin engineers

---

*Last updated: March 2026*
