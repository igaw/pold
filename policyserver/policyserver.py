#
# Policy Daemon - pold
#
# Copyright (C) 2014  BWM Car IT GmbH.
#
# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License version 2 as
# published by the Free Software Foundation.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software
# Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA

import time
import BaseHTTPServer
import base64


HOST_NAME = '127.0.0.1'
PORT_NUMBER = 9000 # Maybe set this to 9000.

# Dictionary of all authorized users. The key is the username, the value
# the password. The policies for a user are stored in a file called
# <username>.policy in this directory. The file must contain a JSON array
# of policies (each policy is a JSON object).
users = {"someuser": "password"}


class MyHandler(BaseHTTPServer.BaseHTTPRequestHandler):
	def do_HEAD(self):
		self.send_response(200)
#		self.send_header("Content-type", "application/json")
		self.send_header("Content-type", "text/html")
		self.end_headers()

	def do_AUTHHEAD(self):
		self.send_response(401)
		self.send_header('WWW-Authenticate', 'Basic realm=\"Policies\"')
		self.send_header("Content-type", "text/html")
		self.end_headers()

	def do_GET(self):
		global user

		if self.headers.getheader("Authorization") == None:
			self.do_AUTHHEAD()
			self.wfile.write("No authorization header received")
			return
		elif self.headers.getheader("Authorization").startswith("Basic "):
			split = self.headers.getheader("Authorization").split(" ")
			assert len(split) == 2
			userpwdbase64 = split[1]

			userpwd = base64.b64decode(userpwdbase64).split(":");
			assert len(userpwd) == 2
			user = userpwd[0]
			pwd = userpwd[1]

			if (users[user] == pwd):
				self.do_HEAD()
				filename = "{}.policies".format(user)
				print "Reading policies from file {}".format(filename)
				with open(filename, 'r') as f:
					self.wfile.write(f.read())
			else:
				self.do_AUTHHEAD()
				self.wfile.write("Wrong username/password")
				return

if __name__ == '__main__':
    server_class = BaseHTTPServer.HTTPServer
    httpd = server_class((HOST_NAME, PORT_NUMBER), MyHandler)
    print time.asctime(), "Server Starts - %s:%s" % (HOST_NAME, PORT_NUMBER)
    try:
        httpd.serve_forever()
    except KeyboardInterrupt:
        pass
    httpd.server_close()
    print time.asctime(), "Server Stops - %s:%s" % (HOST_NAME, PORT_NUMBER)
