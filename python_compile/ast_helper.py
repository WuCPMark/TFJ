import ast
class v(ast.NodeVisitor):
	def generic_visit(self, node):
		print type(node).__name__
		ast.NodeVisitor.generic_visit(self,node)
	def visit_Name(self, node): print 'Name:', node.id


