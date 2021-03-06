#
#  Copyright 2008-2010 NVIDIA Corporation
#  Copyright 2009-2010 University of California
#
#  Licensed under the Apache License, Version 2.0 (the "License");
#  you may not use this file except in compliance with the License.
#  You may obtain a copy of the License at
#
#      http://www.apache.org/licenses/LICENSE-2.0
#
#  Unless required by applicable law or agreed to in writing, software
#  distributed under the License is distributed on an "AS IS" BASIS,
#  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
#  See the License for the specific language governing permissions and
#  limitations under the License.
#

class Context(object):
    def __init__(self, name):
        self.name = name
    def __str__(self):
        return self.name
    def __repr__(self):
        return "<Context: %s>" % self
    def __copy__(self):
        #Enforce singleton
        return self
    def __deepcopy__(self, memo):
        #Enforce singleton
        return self
    
Context.distributed = Context('distributed')
Context.block =    Context('block')
Context.sequential =  Context('sequential')
Context.unknown =     Context('unknown')

distributed = Context.distributed
block = Context.block
sequential = Context.sequential
unknown = Context.unknown


