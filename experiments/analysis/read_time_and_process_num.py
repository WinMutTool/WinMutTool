#!/usr/bin/env python3
import sys
import traceback
import time

now = int(time.monotonic_ns() / 1000000)

class Time:
	def __init__(self, time, tottime, num, totnum, name, unfinished = False):
		self.time = time
		self.tottime = tottime
		self.num = num
		self.totnum = totnum
		self.name = name
		arg = name.split(' ')[1:]
		shortname = name.split(' ')[0].split('/')[-1]
		self.shortname = shortname + ' ' + ' '.join(arg) #name.split('/')[-1].strip()
		self.unfinished = unfinished
	def __str__(self):
		ret = '{:>10.2f} {:>10.2f} {:>10d} {:>10d} {}'.format(self.time, self.tottime, self.num, self.totnum, self.shortname)
		if self.unfinished:
			ret += ' (unfinished)'
		return ret

class TimePair:
	def __init__(self, time1, time2):
		#if time1.shortname.replace('WA', 'DMA') != time2.shortname:
		#	print("warning: shorname not the same:", time1.shortname, time2.shortname)
		# assert time1.shortname.replace('WA', 'DMA') == time2.shortname
		self.shortname1 = time1.shortname
		self.shortname2 = time2.shortname
		self.time1 = time1
		self.time2 = time2
	def __str__(self):
		ret = self.shortname1 + ":\n"
		ret += self.shortname2 + ":\n"
		time1 = self.time1
		time2 = self.time2
		if time1.unfinished:
			ret += ' time1 unfinished\n'
		if time2.unfinished:
			ret += ' time2 unfinished\n'
		if time2.time != 0:
			ret += ' time:    {:>10.2f} / {:>10.2f} = {:>10.2f}% ({:>10.2f})\n'.format(time1.time, time2.time, time1.time / time2.time * 100, time1.time - time2.time)
		else:
			ret += ' time:    {:>10.2f} / {:>10.2f} = {:>11}\n'.format(time1.time, time2.time, 'div 0')
		if time2.tottime != 0:
			ret += ' tottime: {:>10.2f} / {:>10.2f} = {:>10.2f}% ({:>10.2f})\n'.format(time1.tottime, time2.tottime, time1.tottime / time2.tottime * 100, time1.tottime - time2.tottime)
		else:
			ret += ' tottime: {:>10.2f} / {:>10.2f} = {:>11}\n'.format(time1.tottime, time2.tottime, 'div0')
		if time2.num != 0:
			ret += ' num:     {:>10d} / {:>10d} = {:>10.2f}% ({:>10d})\n'.format(time1.num, time2.num, time1.num / time2.num * 100, time1.num - time2.num)
		else:
			ret += ' num:     {:>10d} / {:>10d} = {:>11}\n'.format(time1.num, time2.num, 'div 0')
		if time2.totnum != 0:
			ret += ' totnum:  {:>10d} / {:>10d} = {:>10.2f}% ({:>10d})'.format(time1.totnum, time2.totnum, time1.totnum / time2.totnum * 100, time1.totnum - time2.totnum)
		else:
			ret += ' totnum:  {:>10d} / {:>10d} = {:>11}'.format(time1.totnum, time2.totnum, 'div 0')
		return ret

class Run:
	def __init__(self, casename, timinglist, tottime, totnum, finalFlag):
		self.casename = casename
		self.timinglist = timinglist
		self.tottime = tottime
		self.totnum = totnum
		self.finalFlag = finalFlag
	def __str__(self):
		ret = 'Running time for ' + self.casename + '\n'
		for t in self.timinglist:
			ret += str(t) + '\n'
		if not self.finalFlag:
			ret += 'warning: final flag is not true\n'
		ret += 'tottime:   {:>10.2f}\nnum:       {:>10d}'.format(self.tottime, self.totnum)
		return ret

class RunPair:
	def __init__(self, run1, run2):
		self.run1 = run1
		self.run2 = run2
	def __str__(self):
		run1 = self.run1
		run2 = self.run2
		ret = 'Comparing {} and {}\n'.format(run1.casename, run2.casename)
		for i in range(max(len(self.run1.timinglist), len(self.run2.timinglist))):
			if i >= len(self.run1.timinglist):
				if i == len(self.run1.timinglist):
					ret += 'Only {}\n'.format(str(self.run2.casename))
				ret += str(self.run2.timinglist[i]) + '\n'
			elif i >= len(self.run2.timinglist):
				if i == len(self.run2.timinglist):
					ret += 'Only {}\n'.format(str(self.run1.casename))
				ret += str(self.run1.timinglist[i]) + '\n'
			else:
				ret += str(TimePair(self.run1.timinglist[i], self.run2.timinglist[i])) + '\n'
		return ret
			

def getRun(filename, casename, skip = 0):
	initial_time = None
	totnum = 0
	tottime = 0
	with open(filename, encoding='utf-8', errors='replace') as f:
		timinglist = []
		run = None
		name = None
		time = None
		finaltime = None
		finalFlag = True
		num = 0
		skipped = 0
		for l in f:
			finalFlag = False
			if l.startswith('--'):
				newtime = int(l.split('--')[1].strip())
				newname = '--'.join(l.split('--')[2:]).strip()
				num = 0
				if initial_time is None:
					initial_time = newtime
				time = newtime
				name = newname
			elif l.startswith('++'):
				newtime = int(l.split('++')[1].split('--')[0].strip())
				newname = '--'.join(l.split('--')[1:]).strip()
				if name != newname:
					print(name)
					print(newname)
				#assert name == newname
				if skipped >= skip:
					totnum += num
					tottime += newtime - time
					timinglist.append(Time((newtime - time) / 1000,  tottime / 1000, num, totnum, name))
				else:
					skipped += 1
				finaltime = int(l.split('++')[1].split('--')[0].strip())
				finalFlag = True
			elif l[0] not in '0123456789-+':
				continue
			else:
				num += 1
		
		if finalFlag != True:
			if skipped >= skip:
				timinglist.append(Time((now - time) / 1000, (tottime + now - time) / 1000, num, totnum + num, name, True))
			run = Run(casename, timinglist, tottime / 1000, totnum, finalFlag)
			#for x in timinglist:
			#	print(x)
			#print(str((now - time) / 1000) + '(unfinished)', str((now - initial_time) / 1000) + '(unfinished)', num, totnum + num, name)
			#print('warning: finalFlag not True')
			#print('tottime', (time - initial_time) / 1000)
			#print('num', totnum)
		else:
			#if skipped >= skip:
			#	timinglist.append(Time((finaltime - time) / 1000, (finaltime - initial_time) / 1000, num, totnum + num, name, False))
			run = Run(casename, timinglist, tottime / 1000, totnum, finalFlag)
			#for x in timinglist:
			#	print(x)
			# print((finaltime - time) / 1000, (finaltime - initial_time) / 1000, num, totnum + num, name)
			#totnum += num
			#print('tottime', (finaltime - initial_time) / 1000)
			#print('num', totnum)
		return run

	

try:
	filename = sys.argv[1]
	filename2 = None
	skip = 0
	if len(sys.argv) > 2:
		filename2 = sys.argv[2]
		if (len(sys.argv) > 3):
			skip = int(sys.argv[3])
		print(RunPair(getRun(filename, filename, skip), getRun(filename2, filename2, skip)))
	else:
		print(getRun(filename, filename))
except Exception as e:
	traceback.print_exc(file=sys.stdout)
					
