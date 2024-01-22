#!/usr/bin/env python3
import sys

def parseline(l):
	id = l.split('(')[1].split(')')[0]
	t = int(l.split(':')[3].split('/')[0].strip())
	ret = l.split('/')[1].strip()
	return id, t, ret

def getmap(filename):
	m = {}
	lastline = None
	with open(filename, encoding='utf-8', errors='replace') as f:
		for l in f:
			if l.startswith('--'):
				if lastline is not None:
					yield lastline.split('--')[-1].split('/')[-1], m
				lastline = l
				m = {}
			elif l.startswith('++'):
				continue
			else:
				id, t, ret = parseline(l)
				m[id] = (ret, t)
	if lastline is not None:
		yield lastline.split('--')[-1].split('/')[-1], m

def comparemap(m1, m2):
	onlym1 = {}
	onlym2 = {}
	diff = {}
	for a in m1:
		if a not in m2:
			if m1[a][0] not in onlym1:
				onlym1[m1[a][0]] = set()
			onlym1[m1[a][0]].add(m1[a][1])
	for a in m2:
		if a not in m1:
			if m2[a][0] not in onlym2:
				onlym2[m2[a][0]] = set()
			onlym2[m2[a][0]].add(m2[a][1])
	for a in m1:
		if a in m2:
			if (m1[a][0], m2[a][0]) not in diff:
				diff[(m1[a][0], m2[a][0])] = set()
			diff[(m1[a][0], m2[a][0])].add((m1[a][1], m2[a][1]))
	return onlym1, onlym2, diff

def avg(l):
	return sum(l) / len(l)

if __name__ == '__main__':
	f1 = sys.argv[1]
	f2 = sys.argv[2]
	name = None
	if len(sys.argv) > 3:
		name = sys.argv[3]
	for mp1, mp2 in zip(getmap(f1), getmap(f2)):
		if name is not None:
			if mp1[0].strip() != name:
				continue
		print(mp1[0].strip())
		print(mp2[0].strip())
		m1 = mp1[1]
		m2 = mp2[1]
		onlym1, onlym2, diff = comparemap(m1, m2)
		totm1 = 0
		numm1 = 0
		tot = 0
		num = 0
		print('--ONLY M1--')
		for a in onlym1:
			print(a, sum(onlym1[a]) / len(onlym1[a]), len(onlym1[a]))
			tot += sum(onlym1[a])
			num += len(onlym1[a])
		totm1 += tot
		numm1 += num
		if num != 0:
			print('TOT M1:', tot / num, num)
		totm2 = 0
		numm2 = 0
		tot = 0
		num = 0
		print('--ONLY M2--')
		for a in onlym2:
			print(a, sum(onlym2[a]) / len(onlym2[a]), len(onlym2[a]))
			tot += sum(onlym2[a])
			num += len(onlym2[a])
		totm2 += tot
		numm2 += num
		if num != 0:
			print('TOT M2:', tot / num, num)
		tot1 = 0
		tot2 = 0
		num = 0
		print('--ALL--')
		for a, b in diff:
			atime = 0
			btime = 0
			l = len(diff[(a, b)])
			for x in diff[(a, b)]:
				atime += x[0]
				btime += x[1]
			tot1 += atime
			tot2 += btime
			num += l
			if l != 0:
				print(a, atime/l, '=>', b, btime/l, len(diff[(a, b)]))
		totm1 += tot1
		totm2 += tot2
		numm1 += num
		numm2 += num
		if num != 0:
			print('TOT M1 & M2:', tot1/num, tot2/num, tot1/num - tot2/num, num)
		if numm1 != 0 and numm2 != 0:
			print(totm1 / numm1, totm2 / numm2, 'diff', totm1 / numm1 - totm2 / numm2)
