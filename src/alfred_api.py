#!/bin/false

from json import loads as __load_data,dumps as __save_data

class __EmptyFileData:
	def __repr__(self):
		return "<EmptiFileData: The file was empty>"
	def __iter__(self):
		return iter([])

emptyfiledata=__EmptyFileData()

def __extract_dict(path):
	out=__functions
	for i in path:
		out=out[i]
	return out

__functions={
	"__info__": {
		"lsfunc":( lambda x:list(__extract_dict(x).keys()) ),
		"iotype":( lambda x:"json"),
		"version": (lambda x: [0,0,0,1,0]) # version: major ; minor ; path ; none=0/alfa=1/beta=2 ; alfa/beta version
	}
}

def add_manualy(function,path):
	out=__functions
	for i in path[:-1]:
		if i not in out:
			out[i]=dict()
		out=out[i]
	out[path[-1]]=function

class add:
	def __init__(self,*path,rename=False):
		self.path=path
		self.rename=rename
	def __call__(self,function):
		if self.rename:
			add_manualy(function,self.path)
		else :
			add_manualy(function,self.path + (function.__name__,))
		return function

def call(args):
	try :
		func=__extract_dict(args)
	except :
		with open(1,buffering=0,closefd=False,mode="wb") as fh:
			fh.write(B'[2,"function was not found",""]\n')
		return
	with open(0,buffering=0,closefd=False,mode="rb") as fh:
		data=fh.read()
	if len(data) == 0:
		data=emptyfiledata
	else :
		data=__load_data(data)
	try :
		retval=[0,"",func(data)]
	except Exception as e:
		retval=[10,str(e),""]
	try :
		retval=__save_data(retval,ensure_ascii=True,separators=(',', ':')).encode() # encode is safe because the json has ensure_ascii true
	except :
		retval=B'[1,"",""]'
	with open(1,buffering=0,closefd=False,mode="wb") as fh:
		fh.write(retval)
		fh.write(B"\n")
