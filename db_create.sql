CREATE TABLE db_connect (
	id INTEGER NOT NULL primary key,
	label VARCHAR NOT NULL,
	port INTEGER NOT NULL,
	addr varchar NOT NULL,
	usr VARCHAR,
	psw VARCHAR
);

CREATE TABLE application_br(
	id SERIAL primary key,
	label VARCHAR NOT NULL,
	db INTEGER NOT NULL REFERENCES db_connect(id)
);

CREATE TABLE values_ (
	id SERIAL primary key,
	appname VARCHAR NOT NULL,
	value_id INTEGER NOT NULL,
	value_label VARCHAR NOT NULL,
	value_TYPE VARCHAR NOT NULL
);

CREATE TABLE backup_db(
	id SERIAL,
	content json NOT NULL
);
