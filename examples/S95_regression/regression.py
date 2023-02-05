# Regression testing for Smoldyn
import os
import filecmp
import sys

filelist=['addspecies','abba','allostery','bimolecularlattice','bounce2','bounds1','bounds2','branch',
					'cmdmanipulate','cmdprobability','compartlogic','compartrxn','crowding',
					'diffa','diffuse2b','diffuse2c','diffuse3b','drift','dumbbell',
					'emitter1','hemisphere','lrm','lrmsim','lotvolt','molecule',
					'polymer_end','polymer_mid','rafts','rafts3D','ReflectToNewSpecies',
					'structexpansion','surf1','surf2','surf3','surfaceboundjump','surfacedrift2',
					'surfacedrift3','surfacereact','stick',
					'surfacediffuse','tracking','tracking2','translatemol2','trisphere']

suffix=input('Enter suffix for new simulations: ')
reference=input('Enter suffix for reference simulations or "none": ')

if reference!='none':
	dirname='output_%s/' %(reference)
	if not os.path.exists(dirname):
		print('reference directory "%s" does not exist' %(reference))
		sys.exit()

dirname='output_%s' %(suffix)
try:
	os.makedirs(dirname)
except OSError:
	if os.path.exists(dirname):
		pass
	else:
		raise

for filename in filelist:
	filenew='output_'+suffix+'/'+filename+'_'+suffix+'_out.txt'
	fileref='output_'+reference+'/'+filename+'_'+reference+'_out.txt'
	string='smoldyn %s.txt --define OUTFILE=%s -tqw' %(filename,filenew)
	#	print('system call: %s' %(string))
	os.system(string)
	if reference=='none':
		print('finished %s.txt' %(filename))
	elif not os.path.exists(fileref):
		print('added: %s.txt' %(filename))
	else:
		same=filecmp.cmp(filenew,fileref)
		if same:
			print('same: %s.txt' %(filename))
		else:
			print('DIFFERENT: %s.txt' %(filename))
