import coresyntax as S

class var_visitor(S.SyntaxVisitor):
    def __init__(self, variables):
        super(var_visitor, self).__init__()
	self.variables = variables
	self.varname = ''

    def _Name(self, nm):
        for var in self.variables:
	    if var == nm.id:
	        self.varname = var
        self.visit_children(nm)

class SubVar:
    def __init__(self):
        self.offset = 0
	self.coef = 1 
	self.indvar = ''
        self.dummy = True;

    def setOffset(self, offset):
        self.offset = offset;
        self.dummy = False;

    def setCoef(self, coef):
        self.coef = coef;
        self.dummy = False;

    def setIndvar(self, indvar):
        self.indvar = indvar;
        self.dummy = False;

class index_visitor(S.SyntaxVisitor):
    def __init__(self):
        super(index_visitor, self).__init__()
	self.offset = 0
	self.coef = 1 
	self.indvar = ''
        self.subs = {};
              
    def _Name(self, nm):
        self.indvar = nm.id
        p = SubVar();
        p.setIndvar(nm);
        self.subs[str(nm)] = p;
        
    def _Number(self, num):
        print 'num = %s ' % str(num)

    def _Apply(self, ap):
        gotNum = False;
        name_cnt = 0;
        num_cnt = 0;
        apply_cnt = 0;
        num = -108999;
        name = '';
	if str(ap.function()) == 'ADD':
            #visit all arguments
	    for arg in ap.arguments():
                if arg.__class__.__name__ == 'Number':
                    num_cnt = num_cnt+1;
                    num = arg.val;
                elif arg.__class__.__name__ == 'Name':
                    name_cnt = name_cnt+1;
                    name = str(arg.id);
                elif arg.__class__.__name__ == 'Apply':
                    apply_cnt = apply_cnt+1;
                else:
                    print 'unknown'
                    quit();
                self.visit(arg);

            if(num_cnt == 1 and name_cnt == 1):
                print 'dict size = %d' % len(self.subs)
                m = SubVar();
                m.setOffset(num);
                m.setIndvar(str(name));
                self.subs[str(name)] = m;        
                print 'dict size = %d' % len(self.subs)
              
                
        if str(ap.function()) == 'SUB':
            #visit all arguments
	    for arg in ap.arguments():
                if arg.__class__.__name__ == 'Number':
                    num_cnt = num_cnt+1;
                    num = -arg.val;
                elif arg.__class__.__name__ == 'Name':
                    name_cnt = name_cnt+1;
                    name = str(arg.id);
                elif arg.__class__.__name__ == 'Apply':
                    apply_cnt = apply_cnt+1;
                else:
                    print 'unknown'
                    quit();
                self.visit(arg);

            if(num_cnt == 1 and name_cnt == 1):
                print 'dict size = %d' % len(self.subs)
                m = SubVar();
                m.setOffset(num);
                m.setIndvar(str(name));
                self.subs[str(name)] = m;        
                print 'dict size = %d' % len(self.subs)         


	if str(ap.function()) == 'MUL':
            #visit all arguments
	    for arg in ap.arguments():
                if arg.__class__.__name__ == 'Number':
                    num_cnt = num_cnt+1;
                    num = arg.val;
                elif arg.__class__.__name__ == 'Name':
                    name_cnt = name_cnt+1;
                    name = str(arg.id);
                elif arg.__class__.__name__ == 'Apply':
                    apply_cnt = apply_cnt+1;
                else:
                    print 'unknown'
                    quit();
                self.visit(arg);

            if(num_cnt == 1 and name_cnt == 1 and apply_cnt==0):
                print 'dict size = %d' % len(self.subs)
                m = SubVar();
                m.setCoef(num);
                m.setIndvar(str(name));
                self.subs[str(name)] = m;        
                print 'dict size = %d' % len(self.subs)    
            else:
                print 'need to fix subscript'
                quit();
            

# This visitor creates an XML representation of a procedure
class xmlvisitor(S.SyntaxVisitor):
    
    def __init__(self, fargs, glbs):
        super(xmlvisitor,self).__init__()
        self.fargs = fargs
        self.glbs = glbs
        self.xmlstr = '<?xml version="1.0" encoding="UTF-8" standalone="no"?>\n'
	self.in_subscript = False
	self.variables = []
        self.inductionVars = [];
        self.loadList = [];
        self.loadDict = {};
        self.varSet = set([]);

    	self.dtypemap = {'float32': 'float', 'float64': 'double', 'int32' : 'int'}
    def _Procedure(self, proc):
        self.xmlstr += '<program>\n' 
	self.xmlstr += '<Name>' + str(proc.name()) + '</Name>\n'
        argnum = 0;
        for arg in proc.formals():
            self.xmlstr += '<Var>\n' + '<Name>' + str(arg) + '</Name>\n'
            #import pdb; pdb.set_trace()
            i = self.fargs[argnum]
            self.xmlstr += '<Type>' + self.dtypemap[str(i.dtype)] + '</Type>\n'
            for j in i.shape:
                self.xmlstr += '<Dim>' + str(j) + '</Dim>\n'
            self.xmlstr += '</Var>\n'
	    self.variables.append(str(arg))
            self.varSet.add(str(arg))
            argnum = argnum + 1
        self.visit_children(proc)
        self.xmlstr += '</program>' 

    def _For(self, forstmt):
        self.xmlstr += '<ForStmt>\n'
        self.xmlstr += '<LoopHeader>\n'
        self.xmlstr += '<Name>' + str(id(forstmt)) + '</Name>\n'
        self.xmlstr += '<Induction>' + str(forstmt.indvar()) + '</Induction>\n'
        self.inductionVars.append(str(forstmt.indvar()));
        self.xmlstr += '<Start>' + str(forstmt.lbound()) + '</Start>\n'
        
        strU = str(forstmt.ubound());

        if ((strU in self.varSet)==False) and self.glbs.has_key(strU):
            ubStr = str(self.glbs[strU])
        else:
            ubStr = str(forstmt.ubound());
                                
        self.xmlstr += '<Stop>' + ubStr + '</Stop>\n'
        self.xmlstr += '</LoopHeader>\n'
        self.xmlstr += '<LoopBody>\n'
        self.visit(forstmt.body())
        self.xmlstr += '</LoopBody>\n'
        self.xmlstr += '</ForStmt>\n'
        self.inductionVars.pop();

    def _Bind(self, bindstmt):
        self.visit_children(bindstmt)
	self.visit(bindstmt.binder())

        #add to variable hash table
        self.loadDict[str(bindstmt.binder())] = str(id(bindstmt.value()));
        
        f = 0;
        for l in self.loadList:
            if(l == str(id(bindstmt.binder()))):
                f = 1;
        
        if f==1:        
            self.xmlstr += '<AssignExpr>\n'
            self.xmlstr += '<Name>' + str(id(bindstmt)) + '</Name>\n'
            self.xmlstr += '<Left>' + str(id(bindstmt.binder())) + '</Left>\n'
            self.xmlstr += '<Right>' + str(id(bindstmt.value())) + '</Right>\n'
            self.xmlstr += '</AssignExpr>\n'
#        else:
#            print '<Name>' + str(id(bindstmt)) + '</Name>'
#            print '<Left>' + str(id(bindstmt.binder())) + '</Left>'
#            print '<Right>' + str(id(bindstmt.value())) + '</Right>'

    def _Subscript(self, sub):
        
        if not self.in_subscript:
#            print str(sub) + " not in subscript"

	    self.in_subscript = True
            self.xmlstr += '<MemRef>\n'
            self.xmlstr += '<Name>' + str(id(sub)) + '</Name>\n'
            self.loadList.append(str(id(sub)));

#            print str(id(sub))  
	    vstvar = var_visitor(self.variables)
	    vstvar.visit(sub)
            self.xmlstr += '<Var>' + str(vstvar.varname) + '</Var>\n'
            self.visit_children(sub)
	    self.xmlstr += '</MemRef>\n'
	    self.in_subscript = False
	else:
#            print str(sub) + " in subscript"

	    self.visit_children(sub)

    def _Index(self, ind):
        ind_vst = index_visitor()
	ind_vst.visit(ind)
	self.xmlstr += '<Subscript>\n'

        f = 0;
        sIdx = str(ind_vst.indvar);
 #       print sIdx;
        for ivar in self.inductionVars:
            if sIdx==ivar:
                f = 1;

        if ((f == 0) and (sIdx in self.loadDict)):
            self.xmlstr += '<IndirectRef>' + self.loadDict[sIdx] + '</IndirectRef>\n'
        else:
            print len(ind_vst.subs)
            ostr = '';
            #iterate over dictionary of subscripts
            for key in ind_vst.subs.keys():
                p = ind_vst.subs[key];
                ostr += '<IndVar>' + str(p.indvar) + '</IndVar>\n'
                ostr += '<IndCoeff>' + str(p.coef) + '</IndCoeff>\n'
                ostr += '<Offset>' + str(p.offset) + '</Offset>\n'
            self.xmlstr += ostr;
        
        self.xmlstr += '</Subscript>\n'
        self.visit_children(ind)

    def _Apply(self, ap):
	self.visit_children(ap)
        print str(ap.function())
        if(str(ap.function())=='sel'):
            self.xmlstr += '<SelectExpr>\n'  
            self.xmlstr += '<Name>' + str(id(ap)) + '</Name>\n' 
            self.xmlstr += '<Sel>' + str(id(ap.arguments()[0])) + '</Sel>\n'
            self.xmlstr += '<Left>' + str(id(ap.arguments()[1])) + '</Left>\n'
            self.xmlstr += '<Right>' + str(id(ap.arguments()[2])) + '</Right>\n'
            self.xmlstr += '</SelectExpr>\n'
        elif(str(ap.function())=='log'):
            self.xmlstr += '<UnaryIntrin>\n'  
            self.xmlstr += '<Name>' + str(id(ap)) + '</Name>\n' 
            self.xmlstr += '<Op> log </Op>\n' 
            self.xmlstr += '<Left>' + str(id(ap.arguments()[0])) + '</Left>\n'
            self.xmlstr += '</UnaryIntrin>\n'
        else:
            self.xmlstr += '<CompoundExpr>\n'
            self.xmlstr += '<Name>' + str(id(ap)) + '</Name>\n'
            self.xmlstr += '<Op>' + str(ap.function()) + '</Op>\n'
            self.xmlstr += '<Left>' + str(id(ap.arguments()[0])) + '</Left>\n'
            self.xmlstr += '<Right>' + str(id(ap.arguments()[1])) + '</Right>\n'
            self.xmlstr += '</CompoundExpr>\n'

    def _Number(self, nm):
        if not self.in_subscript:
            self.xmlstr += '<Constant>\n'
	    self.xmlstr += '<Name>' + str(id(nm)) + '</Name>\n'
	    self.xmlstr += '<Value>' + str(nm.val) + '</Value>\n'
	    self.xmlstr += '</Constant>\n'

    def _Sel(self, calls):
        print 'GOT CALLS'
        self.xmlstr += 'FUCK BALLS'

    def _If(self, ifs):
        print 'got if'
        self.visit_children(ifs)
        self.xmlstr += '<SelectExpr>\n'
	self.xmlstr += '<Name>' + str(id(ifs)) + '</Name>\n'
	self.xmlstr += '<Sel>' + str(id(ifs.test())) + '</Sel>\n'
	self.xmlstr += '<Left>' + str(id(ifs.body())) + '</Left>\n'
	self.xmlstr += '<Right>' + str(id(ifs.orelse())) + '</Right>\n'
        self.xmlstr += '</SelectExpr>\n'
