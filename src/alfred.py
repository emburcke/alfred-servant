#!/usr/bin/env python3

import sys
import os
import json
import libeventio as eventio

try :
	os.chdir(os.path.dirname(sys.argv[0]))
except Exception as e: # we runed with -c command. do nothing
	pass

import json

class Locations:
	def __init__(self):
		self.modulepath		= None
		self.classdatapath	= None
		self.addon_index	= None
		self.event_index	= None
#		self.logfile		= None

locations=Locations()
locations.modulepath	= "/usr/local/share/alfred/addons"
locations.classdatapath	= "/usr/local/share/alfred/classave"
locations.addon_index	= "/usr/local/share/alfred/modules.json"
locations.event_index	= "/usr/local/share/alfred/events.json"
#locatiosn.logfile	= "data/main.log"

def toexecargv(file,params):
	out=[file.encode()]
	for i in params:
		out.apend(i.encode())
	return out


class Modules:
	def __init__(self,locates=locations):
		self.childs=dict()
		if not os.path.exist(locates.addon_index):
			with open(locates.addon_index,"w") as fh:
				json.dump(fh,{})
		with open(locates.addon_index) as fh:
			for name,i in json.load(fh).items():
				self.childs[str(name)]=	[	toexecargv(
									os.path.abspath(os.path.join(
										i["path"] if "path" in i else locates.modulepath,
										i["file"]
									)),
									i["function"] if "function" in i else []
								),
								i["class"] if "class" in i else None,
								i["mode"] if "mode" in i else "stdio",
							]
	def call(self,child,data):
		child=self.childs[child]
		if child[2] == "stdio": # we dont support other methods than fopen
			if None is child[1]: # we dont support classes right now, only functions
				return eventio.call_child(child[0],json.dumps(data,ensure_ascii=True) + "\n")
	def existchild(self,name):
		return name in self.childs

modules=Modules()

class blockingfor:
	instanced=False
	def __new__(cls,*args,**kwargs):
		if cls.instanced:
			raise RuntimeError("this class is a wrapper for a c module, so you can not reinstance it")
		cls.instanced=True
		return super().__new__(cls,*args,**kwargs)
	def __iter__(self):
		return self
	def __next__(self):
		return self.get()
	def get(self):
		while True: # only return if there is a good return value
			out=eventio.blockingfor_pyget()
			if isinstance(out[1],int): # we do the read here, because python handles reading well, and raises all kind of errors if something is failed
				try :
					with open(out[1],"rb") as fh:
						data=json.load(fh)
						value=[int(data[0]),str(data[1]),data[2]]
						if value[0] == 0:	# we check for errors, but silently ignore them, so only return if the code is good
							return out[0],value[2]
				except Exception as e:
					pass
			else :
				try :
					data=json.loads(out[1])
					return out[0],data
				except Exception as e:
					pass
	def add(self,value): # we want to replace all functions that call this, with functions that calls the c function
		raise NotImplementedError("Currently, i'm too lasy to develop this function")
	def multiadd(self,values):
		for i in values:
			self.add(i)

blockingfor=blockingfor() # single instancing it

class Events:
	def __init__(self,addons=modules,locates=locations):
		self.events=dict()
		if not os.path.exist(locates.event_index):
			with open(locates.event_index,"w") as fh:
				json.dump(fh,[])
		with open(locates.event_index) as fh:
			for event,*assigns in json.load(fh):
				self.events[event]=set()
				for i in assigns:
					if addons.existchild(i):
						self.events[event].add(i)
					else :
						pass # TODO log, there is no child with that name
		self.addons = addons
		eventio.listen_to_message(B"/alfredio")
	def main_loop(self):
		for pid,event in blockingfor:
			print("pid:",pid,"event:",event)
			try :
				eventname=event["name"]
				data=event["data"]
			except :
				continue # TODO, itt jarok, es TODO a child converting elesett valamivel, aka rosz output, tehat nem tamogatja a child az apit
			if eventname in self.events:
				for i in self.events[eventname]:
					child_pid=self.addons.call(i,data)
			if eventname == "exit":
				return

if __name__ == "__main__":
	events=Events()
	events.main_loop()
