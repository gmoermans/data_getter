from tkinter import *
#from tkinter.ttk import *
import psycopg2

def extractName(postString):
	result = ""
	for i in postString:
		if ord(i) >= ord('a') and ord(i) <= ord('z'):
			result = result+i
		elif ord(i) >= ord('0') and ord(i) <= ord('9'):
			result = result+i
		elif ord(i) >= ord('A') and ord(i) <= ord('Z'):
			result = result + i
		elif ord(i) == ord(']') or ord(i) == ord('['):
			result = result + i
	return result 
	
def resQuerry(query):#change into non transactionnal
	try:
		conn = psycopg2.connect("host=localhost port=5432 dbname=chirpstack_as user=postgres password=dbpassword")
		dbcursor = conn.cursor()
		dbcursor.execute(query)
		apps_arr = []
		if dbcursor.rowcount == 0:
			apps_arr.append('empty')
			return apps_arr

		rows = dbcursor.fetchall()
		for row in rows:
			apps_arr.append( row );
		dbcursor.close()
		return apps_arr
	except (Exception, psycopg2.DatabaseError) as error:
		print(error)
	finally:
		if conn is not None:
		    conn.close()
		    print('Database connection closed.')

def insertInto( qr ):
	result = ()
	try:
		conn = psycopg2.connect("host=localhost port=5432 dbname=chirpstack_as user=postgres password=dbpassword")
		dbcursor = conn.cursor()
		dbcursor.execute(qr)
		conn.commit()
		result =  dbcursor.fetchone()
		conn.close()
		return result
	except (Exception, psycopg2.DatabaseError) as error:
		print(error)
	finally:
		if conn is not None:
		    conn.close()
		    print('Database connection closed.')

def deleteFrom( qr ):
	try:
		conn = psycopg2.connect("host=localhost port=5432 dbname=chirpstack_as user=postgres password=dbpassword")
		dbcursor = conn.cursor()
		dbcursor.execute(qr)
		conn.commit()
		conn.close()
	except (Exception, psycopg2.DatabaseError) as error:
		print(error)
	finally:
		if conn is not None:
		    conn.close()
		    print('Database connection closed.')
if __name__ == '__main__':

	#predeclare variables
	dbSelectionned = ""
	root = Tk()
	root.title("insert value into database")

	# Add a grid
	mainframe = Frame(root)
	mainframe.grid(column=0,row=0, sticky=(N,W,E,S) )
	mainframe.columnconfigure(0, weight = 1)
	mainframe.rowconfigure(0, weight = 1)
	mainframe.pack(pady = 100, padx = 100)

	# Create a Tkinter variable for application selection
	tkvar = StringVar(root)

	# Create a Tkinkter variable for name in text field
	vname = StringVar(root)

	# create variable for new application name
	appNameString = StringVar( root )
	# Create a Tkinkter variable for delete in database.
	deletename = StringVar(root)

	#create variable for manual delete
	manualdelete = StringVar(root)

	# Tkinter variable for type
	valuetype = StringVar(root)

	# for the id of value ( GUI need )
	idforuser = StringVar(root)

	# for the db selector dropdown
	physicalDataBase = StringVar( root )

	#variable to get id
	seemyid = StringVar( root )

	#variable to type value
	whatismyid = StringVar( root )

	apps = resQuerry('SELECT label FROM application_br')
	tkvar.set('apps available') # set the default option

	tps = ('integer', 'float', 'date', 'time', 'string')
	#valueType.set('float') # set the default type value ( float works everytime )

	#dropdown menu for every available applications
	appMenu = OptionMenu(mainframe, tkvar, *apps)
	Label(mainframe, text=" select app").grid(row = 0, column = 0)
	appMenu.grid(row = 1, column = 0)

	#dropdown menu for data types
	Label(mainframe, text="type for new value").grid(row = 0, column = 2)
	typeMenu = OptionMenu(mainframe, valuetype, *tps)
	typeMenu.grid(row = 1, column = 2)
	
	#dropdown menu for physical database
	dbs = ('lilliad',)
	Label(mainframe, text="select database").grid(row = 4, column = 0)
	physicalDbMenu = OptionMenu(mainframe, physicalDataBase, *dbs )
	physicalDbMenu.grid( row = 5, column = 0 )

	# on when value change on application choice dropdown
	def changeAppDropDown(*args):
		dbSelectionned = tkvar.get()
		qstring = 'SELECT value_label, value_id FROM values_ WHERE appname = \''+extractName(dbSelectionned)+'\''
		print( 'query is : ', qstring )
		allValues = resQuerry(qstring)

		#dropdown for data in selectionned database ( dynamic setting )
		Label(mainframe, text="select value to delete").grid(row = 2, column = 0)
		valuesList = OptionMenu(mainframe, deletename, *allValues )
		valuesList.grid( row = 3, column = 0 )
		
	# link function to change dropdown
	tkvar.trace('w', changeAppDropDown)

	def getNextId(appName):
		qr = "SELECT MAX(value_id)+1 FROM values_ where appname ='"+appName
		qr = qr + "';"
		var = resQuerry(qr)
		comp = var[0][0]
		print( 'var : ',var )
		if comp is None:
			return [(0,)]#if id was 0
		return var

	def delClick():
		delval = ""
		if len( manualdelete.get() ) > 1:
			delval = manualdelete.get()
		else:
			delval = deletename.get()
		qr = "delete from values_ where appname = '"
		qr = qr + extractName(tkvar.get())
		qr = qr +"' and value_label = '"
		qr = qr + extractName(delval)
		qr = qr + "';"
		print(qr)
		deleteFrom( qr )

	def addClick():
		newvalue = extractName(vname.get())
		dbselectionned = extractName(tkvar.get())
		nextId = getNextId(dbselectionned)[0][0]
		querry = "INSERT INTO values_( appname, value_id, value_label, value_type ) VALUES('"+dbselectionned
		querry = querry +"',"
		querry = querry +str(nextId)
		querry = querry +",'"
		querry = querry +newvalue
		querry = querry +"','"
		querry = querry +valuetype.get()
		querry = querry +"' ) RETURNING value_id;" # will need the return of this...
		confirmationId = insertInto(querry)[0]
		idforuser.set('id for new value : '+str(confirmationId))

	def addApp():
		print('add app click')
		tempAppName = extractName(appNameString.get())

		#Get the id of physical db destination
		q1 = "SELECT id FROM db_connect WHERE label = '";
		q1 = q1 +  extractName(physicalDataBase.get())
		q1 = q1 + "';"
		print("query to get id ", q1)
		idOfDatabase = resQuerry(q1)[0][0]
		print("q1 result is : ", idOfDatabase)		
		#prepare the querry for inserting object
		q2 = "INSERT INTO application_br( label, db ) VALUES('"
		q2 = q2 + tempAppName
		q2 = q2 + "',"
		q2 = q2 + str(idOfDatabase)
		q2 = q2 + ");"		
		insertInto( q2 )

	def idClick():
		idTemName = extractName( whatismyid.get() )
		q1 = "SELECT value_label, value_id FROM values_ WHERE value_label = '"
		q1 = q1 + idTemName
		q1 = q1 + "' and appname = '"
		q1 = q1 + extractName(tkvar.get())
		q1 = q1 + "';"
		r1 = resQuerry(q1)
		print('querry is' , q1)
		print('result is', r1)
		seemyid.set("Id of value is : " + str(r1))
	
	Label(mainframe, text="name for new value").grid(row = 0, column = 1)
	valuename = Entry(mainframe, width = 15, textvariable = vname )
	valuename.grid(row = 1, column = 1 ) 
	addValue = Button(mainframe, text = "send value", command = addClick )
	addValue.grid( row = 1, column = 4 )

	deleteTextField = Entry(mainframe, width = 15, textvariable = manualdelete )
	deleteTextField.grid( row = 3, column = 1 )
	deleteValue = Button(mainframe, text = "delete value", command = delClick )
	deleteValue.grid( row = 3, column = 2 )

	Label(mainframe, text="name for newapplication").grid(row = 4, column = 1)
	newAppName = Entry(mainframe, width = 15, textvariable = appNameString )
	newAppName.grid(row = 5, column = 1 )  
	addApplication = Button(mainframe, text = "add application", command = addApp )
	addApplication.grid( row = 5, column = 2 )

	idLabel = Label( root, textvariable=idforuser, relief=RAISED )
	idforuser.set('id for new value : ')
	idLabel.pack()

	Label(mainframe, text="type value to see his ID").grid(row = 6, column = 1)
	seeMyIdField = Entry(mainframe, width = 15, textvariable = whatismyid )
	seeMyIdField.grid( row=7, column = 1)
	clickToSeeId = Button(mainframe, text = "see id", command = idClick )
	clickToSeeId.grid( row = 7, column = 2 )

	myIdLabel = Label( root, textvariable=seemyid, relief=RAISED )
	myIdLabel.pack()

	root.mainloop()
