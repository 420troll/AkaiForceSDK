#!/usr/bin/env python3
"""Compare original ASR-10 ES5510 core and standalone MAME adapter.

Generated stimuli, CSV audio, and summary are written under --out. Factory
payloads are not consumed by this tool.
"""
import argparse
import csv
import json
import math
import pathlib
import subprocess

ROOT = pathlib.Path(__file__).resolve().parents[2]

def clamp(value):
    return max(-32768, min(32767, round(value)))

def stimuli(rate, count):
    result = {}
    impulse = [(0,0)] * count
    impulse[count//8] = (24000,0)
    result['impulse'] = impulse
    result['sine'] = [(clamp(24000*math.sin(2*math.pi*997*i/rate)),)*2 for i in range(count)]
    sweep=[]; phase=0.0
    for i in range(count):
        freq=30*(rate/2/30)**(i/max(1,count-1)); phase += 2*math.pi*freq/rate
        sample=clamp(18000*math.sin(phase)); sweep.append((sample,sample))
    result['sweep']=sweep
    state=0x5510a55a; noise=[]
    for _ in range(count):
        pair=[]
        for _channel in range(2):
            state ^= (state << 13) & 0xffffffff; state ^= state >> 17; state ^= (state << 5) & 0xffffffff
            pair.append(((state & 0xffff)-32768)//2)
        noise.append(tuple(pair))
    result['noise']=noise
    result['stereo_phase']=[(clamp(22000*math.sin(2*math.pi*431*i/rate)),
                             clamp(22000*math.sin(2*math.pi*431*i/rate+math.pi/2))) for i in range(count)]
    return result

def read_outputs(path):
    with path.open(newline='') as f:
        return [[int(row['out_l']),int(row['out_r'])] for row in csv.DictReader(f)]

def metrics(reference, candidate):
    count=min(len(reference),len(candidate))
    errors=[candidate[i][ch]-reference[i][ch] for i in range(count) for ch in range(2)]
    best=(0,float('inf')); limit=min(128,max(1,count//4))
    for lag in range(-limit,limit+1):
        pairs=[(reference[i][ch],candidate[i+lag][ch]) for i in range(count) for ch in range(2) if 0<=i+lag<count]
        mse=sum((b-a)**2 for a,b in pairs)/max(1,len(pairs))
        if mse<best[1]: best=(lag,mse)
    def channel(ch):
        ref=[reference[i][ch] for i in range(count)]; got=[candidate[i][ch] for i in range(count)]
        return {'reference_peak':max(map(abs,ref),default=0),'candidate_peak':max(map(abs,got),default=0),
                'reference_rms':math.sqrt(sum(x*x for x in ref)/max(1,count)),
                'candidate_rms':math.sqrt(sum(x*x for x in got)/max(1,count))}
    def timing(samples):
        active=next((i for i,row in enumerate(samples) if row[0] or row[1]),None)
        peak=max(range(count),key=lambda i:max(map(abs,samples[i]))) if count else 0
        return {'first_nonzero_frame':active,'peak_frame':peak}
    first=next((i for i in range(count) if reference[i]!=candidate[i]),None)
    return {'frames':count,'bit_exact':len(reference)==len(candidate) and first is None,
            'first_divergent_frame':first,'max_abs_deviation':max(map(abs,errors),default=0),
            'rms_deviation':math.sqrt(sum(e*e for e in errors)/max(1,len(errors))),
            'best_alignment_lag_frames':best[0],'aligned_rms_error':math.sqrt(best[1]),
            'channels':{'left':channel(0),'right':channel(1)},
            'stereo_difference_reference_rms':math.sqrt(sum((reference[i][0]-reference[i][1])**2 for i in range(count))/max(1,count)),
            'stereo_difference_candidate_rms':math.sqrt(sum((candidate[i][0]-candidate[i][1])**2 for i in range(count))/max(1,count)),
            'reference_timing':timing(reference),'candidate_timing':timing(candidate)}

def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--reference-root',type=pathlib.Path,required=True,
                        help='ASR-10 emulator checkout containing src/es5510.cpp and src/es5510.h')
    parser.add_argument('--out',type=pathlib.Path,default=pathlib.Path('/tmp/es5510-audio-validation'))
    parser.add_argument('--rate',type=int,default=44100)
    parser.add_argument('--frames',type=int,default=8192)
    args=parser.parse_args()
    if not 8000<=args.rate<=192000 or not 1<=args.frames<=1_000_000:
        parser.error('rate or frame count outside supported range')
    ref=args.reference_root.resolve()
    for required in (ref/'src/es5510.cpp',ref/'src/es5510.h'):
        if not required.is_file(): parser.error(f'missing reference source: {required}')
    args.out.mkdir(parents=True,exist_ok=True)
    exe=args.out/'es5510-audio-compare'
    sources=[ref/'src/es5510.cpp',ROOT/'experimental/emulation/mame_es5510_standalone.cpp',ROOT/'tools/audio_validation/compare.cpp']
    subprocess.run(['c++','-std=c++17','-O2','-Wall','-Wextra','-Wpedantic','-Werror',
                    '-I'+str(ref/'src'),'-I'+str(ROOT/'experimental/emulation'),
                    *map(str,sources),'-o',str(exe)],cwd=ROOT,check=True)
    summaries={}
    for name,samples in stimuli(args.rate,args.frames).items():
        stimulus=args.out/f'{name}.csv'
        with stimulus.open('w',newline='') as f: csv.writer(f).writerows(samples)
        summaries[name]={}
        for algorithm in ('pass','delay'):
            outputs={}
            for engine in ('original','mame'):
                path=args.out/f'{name}-{algorithm}-{engine}.csv'
                subprocess.run([str(exe),engine,algorithm,str(stimulus),str(path)],check=True)
                outputs[engine]=read_outputs(path)
            summary=metrics(outputs['original'],outputs['mame'])
            input_start=next((i for i,row in enumerate(samples) if row[0] or row[1]),None)
            summary['input_first_nonzero_frame']=input_start
            for engine in ('original','mame'):
                output_start=next((i for i,row in enumerate(outputs[engine]) if row[0] or row[1]),None)
                summary[f'{engine}_input_to_output_delay_frames']=(output_start-input_start
                    if output_start is not None and input_start is not None else None)
            if name=='impulse' and algorithm=='delay':
                summary['impulse_response_taps']={engine:[
                    {'frame':i,'offset_from_input_frames':i-input_start,'left':row[0],'right':row[1]}
                    for i,row in enumerate(outputs[engine]) if row[0] or row[1]] for engine in outputs}
            summaries[name][algorithm]=summary
    report={'sample_rate_hz':args.rate,'frames_per_stimulus':args.frames,
            'comparison':'ASR-10 emulator ES5510 vs standalone TROLLfx MAME-derived adapter',
            'summaries':summaries}
    (args.out/'summary.json').write_text(json.dumps(report,indent=2)+'\n')
    print(json.dumps(report,indent=2))

if __name__=='__main__': main()
