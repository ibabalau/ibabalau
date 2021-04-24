DROP TABLE IF EXISTS temperaturi;
DROP TABLE IF EXISTS orase;
DROP TABLE IF EXISTS tari;

CREATE TABLE tari
(
    id SERIAL PRIMARY KEY,
    nume text UNIQUE,
    lat FLOAT NOT NULL,
    lon FLOAT NOT NULL
);

CREATE TABLE orase (
    id SERIAL PRIMARY KEY,
    idTara INTEGER,
    nume text,
    lat FLOAT NOT NULL,
    lon FLOAT NOT NULL,
    UNIQUE (idTara, nume),
    FOREIGN KEY (idTara)
        REFERENCES tari(id)
        ON UPDATE CASCADE ON DELETE CASCADE
);

CREATE TABLE temperaturi (
    id SERIAL PRIMARY KEY,
    valoare FLOAT NOT NULL,
    timestamp TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    idOras INTEGER,
    UNIQUE (idOras, timestamp),
    FOREIGN KEY (idOras)
        REFERENCES orase(id)
        ON UPDATE CASCADE ON DELETE CASCADE
);
