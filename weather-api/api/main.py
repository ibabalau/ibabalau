from flask import Flask, request, jsonify, Response, make_response
import psycopg2
from psycopg2.extras import RealDictCursor
import os

app = Flask('TEMA2_SPRC')
app.config['JSON_SORT_KEYS'] = False

COUNTRIES_ROUTE = '/api/countries'
CITIES_ROUTE = '/api/cities'
TEMPS_ROUTE = '/api/temperatures'

DB_USER = os.getenv('DB_USER')
DB_PASS = os.getenv('DB_PASS')
DB_HOSTNAME = os.getenv('DB_HOSTNAME')
DB_NAME = os.getenv('DB_DB')

def isInt(param):
    try:
        if isinstance(param, int):
            return True
        return False
    except ValueError:
        return False

def isStr(param):
    try:
        if isinstance(param, str):
            return True
        return False
    except ValueError:
        return False

# Returns true if param is a int or float
def isValidFloat(param):
    try:
        if isinstance(float(param), float):
            return True
        return False
    except ValueError:
        return False


def exec_db_cmd_ret_id(cmd, args):
    database_connection = psycopg2.connect(host=DB_HOSTNAME,
                                           database=DB_NAME,
                                           user=DB_USER,
                                           password=DB_PASS)
    database_connection.set_session(autocommit=True)
    ret = -1
    cur = database_connection.cursor()
    try:
        cur.execute(cmd, args)
        ret = cur.fetchone()[0]
    except (Exception, psycopg2.DatabaseError) as error:
        print(error)
    cur.close()
    database_connection.close()
    return ret


def check_country_id(country_id):
    database_connection = psycopg2.connect(host=DB_HOSTNAME,
                                           database=DB_NAME,
                                           user=DB_USER,
                                           password=DB_PASS)
    database_connection.set_session(autocommit=True)
    ret = -1
    cur = database_connection.cursor()
    try:
        cur.execute("select * from tari where id = " + str(country_id))
        ret = cur.fetchone()[0]
    except (Exception, psycopg2.DatabaseError) as error:
        print(error)
    cur.close()
    database_connection.close()
    return ret


def check_city_id(city_id):
    database_connection = psycopg2.connect(host=DB_HOSTNAME,
                                           database=DB_NAME,
                                           user=DB_USER,
                                           password=DB_PASS)
    database_connection.set_session(autocommit=True)
    ret = -1
    cur = database_connection.cursor()
    try:
        cur.execute("select * from orase where id = " + str(city_id))
        ret = cur.fetchone()[0]
    except (Exception, psycopg2.DatabaseError) as error:
        print(error)
    cur.close()
    database_connection.close()
    return ret


def exec_db_cmd_ret_rowcnt(cmd_str, args):
    database_connection = psycopg2.connect(host=DB_HOSTNAME,
                                           database=DB_NAME,
                                           user=DB_USER,
                                           password=DB_PASS)
    database_connection.set_session(autocommit=True)
    updated_rows = -1
    cur = database_connection.cursor()
    try:
        cur.execute(cmd_str, args)
        updated_rows = cur.rowcount
    except (Exception, psycopg2.DatabaseError) as error:
        print(error)
    cur.close()
    database_connection.close()
    return updated_rows


@app.route(COUNTRIES_ROUTE, methods=['POST'])
def add_country():
    payload = request.get_json(silent=True)
    if not payload:
        return Response(status=400)
    json_params = ['nume', 'lon', 'lat']
    for param in json_params:
        if param not in payload:
            return Response(status=400)
    if not isValidFloat(payload['lat']):
        return Response(status=400)
    elif not isValidFloat(payload['lon']):
        return Response(status=400)
    elif not isStr(payload['nume']):
        return Response(status=400)
    cmd = """INSERT INTO tari(nume, lat, lon)
                 VALUES(%s, %s, %s) RETURNING id;"""
    args = (payload['nume'], payload['lat'], payload['lon'],)
    id = exec_db_cmd_ret_id(cmd, args)
    if id == -1:
        return Response(status=409)
    response = make_response(
        jsonify(
            {'id': id}
        ),
        201,
    )
    return response


@app.route(CITIES_ROUTE, methods=['POST'])
def add_city():
    payload = request.get_json(silent=True)
    if not payload:
        return Response(status=400)
    json_params = ['idTara', 'nume', 'lon', 'lat']
    for param in json_params:
        if param not in payload:
            return Response(status=400)
    if not isValidFloat(payload['lat']):
        return Response(status=400)
    elif not isValidFloat(payload['lon']):
        return Response(status=400)
    elif not isStr(payload['nume']):
        return Response(status=400)
    elif not isInt(payload['idTara']):
        return Response(status=400)
    # add to db
    if check_country_id(payload['idTara']) == -1:
        return Response(status=404)
    cmd = """INSERT INTO orase(idtara, nume, lat, lon)
                 VALUES(%s, %s, %s, %s) RETURNING id;"""
    args = (payload['idTara'], payload['nume'], payload['lat'], payload['lon'],)
    id = exec_db_cmd_ret_id(cmd, args)
    if id == -1:
        return Response(status=409)
    response = make_response(
        jsonify(
            {'id': id}
        ),
        201,
    )
    return response


@app.route(TEMPS_ROUTE, methods=['POST'])
def add_temp():
    payload = request.get_json(silent=True)
    if not payload:
        return Response(status=400)
    json_params = ['idOras', 'valoare']
    for param in json_params:
        if param not in payload:
            return Response(status=400)
    if not isInt(payload['idOras']):
        return Response(status=400)
    elif not isValidFloat(payload['valoare']):
        return Response(status=400)
    if check_city_id(payload['idOras']) == -1:
        return Response(status=404)
    cmd = 'INSERT INTO temperaturi(valoare, idoras) VALUES(%s, %s) RETURNING id;'
    args = (payload['valoare'], payload['idOras'],)
    id = exec_db_cmd_ret_id(cmd, args)
    if id == -1:
        return Response(status=409)
    response = make_response(jsonify({'id': id}), 201)
    return response

# execute a select and return a dict of values with columns as keys
def db_select_formated(query, columns):
    database_connection = psycopg2.connect(host=DB_HOSTNAME,
                                           database=DB_NAME,
                                           user=DB_USER,
                                           password=DB_PASS)
    database_connection.set_session(autocommit=True)
    cur = database_connection.cursor()
    cur.execute(query)
    results = []
    for row in cur.fetchall():
        results.append(dict(zip(columns, row)))
    cur.close()
    database_connection.close()
    return results


# execute a select and return a dict of values database column names as keys
def db_select(query):
    database_connection = psycopg2.connect(host=DB_HOSTNAME,
                                           database=DB_NAME,
                                           user=DB_USER,
                                           password=DB_PASS)
    database_connection.set_session(autocommit=True)
    cur = database_connection.cursor(cursor_factory=RealDictCursor)
    cur.execute(query)
    results = cur.fetchall()
    cur.close()
    database_connection.close()
    return results


@app.route(COUNTRIES_ROUTE, methods=['GET'])
def get_countries():
    results = db_select('SELECT * from tari order by id asc')
    return make_response(
        jsonify(results),
        200
    )

@app.route(CITIES_ROUTE, methods=['GET'])
def get_cities_all():
    cmd = 'SELECT id, idtara, nume, lat, lon from orase order by id asc'
    columns = ('id', 'idTara', 'nume', 'lat', 'lon')
    results = db_select_formated(cmd, columns)
    return make_response(
        jsonify(results),
        200
    )


@app.route(CITIES_ROUTE + '/country/<int:country_id>', methods=['GET'])
def get_cities_from_country(country_id):
    results = db_select('SELECT id, nume, lat, lon from orase where idtara=' + str(country_id)) + ' order by id asc'
    return make_response(
        jsonify(results),
        200
    )


@app.route(COUNTRIES_ROUTE + '/<int:id>', methods=['PUT'])
def modify_country(id):
    payload = request.get_json(silent=True)
    if not payload:
        return Response(status=400)
    json_params = ['id', 'nume', 'lon', 'lat']
    for param in json_params:
        if param not in payload:
            return Response(status=400)
    if id != payload['id']:
        return Response(status=400)
    if not isValidFloat(payload['lat']):
        return Response(status=400)
    elif not isValidFloat(payload['lon']):
        return Response(status=400)
    elif not isInt(payload['id']):
        return Response(status=400)
    elif not isStr(payload['nume']):
        return Response(status=400)
    # modify in db
    cmd = """   UPDATE tari
                SET nume = %s, lat = %s, lon = %s
                WHERE id = %s"""
    args = (payload['nume'], payload['lat'], payload['lon'], id, )
    ret = exec_db_cmd_ret_rowcnt(cmd, args)
    if ret == 0:
        return Response(status=404)
    elif ret == -1:
        return Response(status=409)
    return Response(status=200)

@app.route(CITIES_ROUTE + '/<int:id>', methods=['PUT'])
def modify_city(id):
    payload = request.get_json(silent=True)
    if not payload:
        return Response(status=400)
    json_params = ['id', 'idTara', 'nume', 'lon', 'lat']
    for param in json_params:
        if param not in payload:
            return Response(status=400)
    if id != payload['id']:
        return Response(status=400)
    if not isValidFloat(payload['lat']):
        return Response(status=400)
    elif not isValidFloat(payload['lon']):
        return Response(status=400)
    elif not isInt(payload['id']):
        return Response(status=400)
    elif not isInt(payload['idTara']):
        return Response(status=400)
    elif not isStr(payload['nume']):
        return Response(status=400)
    # check for conflicts
    ret = (payload['idTara'], payload['nume'])
    cmd = """   UPDATE orase
                SET idTara = %s, nume = %s, lat = %s, lon = %s
                WHERE id = %s"""
    args = (payload['idTara'], payload['nume'], payload['lat'], payload['lon'], id, )
    ret = exec_db_cmd_ret_rowcnt(cmd, args)
    print(ret)
    if ret == 0:
        return Response(status=404)
    elif ret == -1:
        return Response(status=409)
    return Response(status=200)


@app.route(TEMPS_ROUTE + '/<int:id>', methods=['PUT'])
def modify_temp(id):
    payload = request.get_json(silent=True)
    if not payload:
        return Response(status=400)
    json_params = ['id', 'idOras', 'valoare']
    for param in json_params:
        if param not in payload:
            return Response(status=400)
    if id != payload['id']:
        return Response(status=400)
    if not isInt(payload['idOras']):
        return Response(status=400)
    elif not isInt(payload['id']):
        return Response(status=400)
    elif not isValidFloat(payload['valoare']):
        return Response(status=400)
    # modify in db
    cmd = """ UPDATE temperaturi
                        SET idoras = %s, valoare = %s
                        WHERE id = %s"""
    args = (payload['idOras'], payload['valoare'], id, )
    ret = exec_db_cmd_ret_rowcnt(cmd, args)
    if ret <= 0:
        return Response(status=404)
    return Response(status=200)


@app.route(COUNTRIES_ROUTE + '/<int:id>', methods=['DELETE'])
def delete_country(id):
    rows_deleted = exec_db_cmd_ret_rowcnt('DELETE from tari where id= %s', (id, ))
    if rows_deleted == 0:
        return Response(status=404)
    return Response(status=200)


@app.route(CITIES_ROUTE + '/<int:id>', methods=['DELETE'])
def delete_city(id):
    rows_deleted = exec_db_cmd_ret_rowcnt('DELETE from orase where id= %s', (id,))
    if rows_deleted == 0:
        return Response(status=404)
    return Response(status=200)


@app.route(TEMPS_ROUTE + '/<int:id>', methods=['DELETE'])
def delete_temp(id):
    rows_deleted = exec_db_cmd_ret_rowcnt('DELETE from temperaturi where id= %s', (id,))
    if rows_deleted == 0:
        return Response(status=404)
    return Response(status=200)


@app.route(TEMPS_ROUTE, methods=['GET'])
def get_temps():
    lat = request.args.get('lat')
    lon = request.args.get('lon')
    if lat and not isValidFloat(lat):
        return make_response(jsonify([]), 200)
    if lon and not isValidFloat(lon):
        return make_response(jsonify([]), 200)
    from_date = request.args.get('from')
    if not from_date:
        from_date = '\'2020-01-01\''
    else:
        from_date = '\'' + from_date + '\''
    until_date = request.args.get('until')
    if not until_date:
        until_date = 'current_timestamp'
    else:
        until_date = '\'' + until_date + '\''
    query_str = 'select t.id, t.valoare, t.timestamp from temperaturi t '
    lat_lon_str = ''
    if lat or lon:
        query_str += 'inner join orase o on t.idoras = o.id where '
        if lat and lon:
            lat_lon_str = lat_lon_str + 'o.lat = ' + lat + ' and ' + 'o.lon = ' + lon + ' and '
        elif lat:
            lat_lon_str = lat_lon_str + 'o.lat = ' + lat + ' and '
        elif lon:
            lat_lon_str = lat_lon_str + 'o.lon = ' + lon + ' and '
    else:
        query_str += 'where '
    query_str = query_str + lat_lon_str + 't.timestamp >= ' + from_date + ' and t.timestamp <= ' + until_date + ' order by id asc'
    try:
        results = db_select(query_str)
    except (Exception, psycopg2.DatabaseError) as error:
        print(error)
        return make_response(jsonify([]), 200)
    return make_response(jsonify(results), 200)


@app.route(TEMPS_ROUTE + '/cities/<int:city_id>', methods=['GET'])
def get_temps_for_city(city_id):
    from_date = request.args.get('from')
    if not from_date:
        from_date = '\'2020-01-01\''
    else:
        from_date = '\'' + from_date + '\''
    until_date = request.args.get('until')
    if not until_date:
        until_date = 'current_timestamp'
    else:
        until_date = '\'' + until_date + '\''
    query_str = 'select t.id, t.valoare, t.timestamp from temperaturi t ' \
                'inner join orase o on t.idoras = o.id where o.id = ' \
                + str(city_id) + ' and ' + 't.timestamp >= ' + from_date \
                + ' and t.timestamp <= ' + until_date + ' order by id asc'
    try:
        results = db_select(query_str)
    except (Exception, psycopg2.DatabaseError) as error:
        print(error)
        return make_response(jsonify([]), 200)
    return make_response(jsonify(results), 200)


@app.route(TEMPS_ROUTE + '/countries/<int:country_id>', methods=['GET'])
def get_temps_for_country(country_id):
    from_date = request.args.get('from')
    if not from_date:
        from_date = '\'2020-01-01\''
    else:
        from_date = '\'' + from_date + '\''
    until_date = request.args.get('until')
    if not until_date:
        until_date = 'current_timestamp'
    else:
        until_date = '\'' + until_date + '\''
    query_str = 'select t.id, t.valoare, t.timestamp from temperaturi t ' \
                'inner join orase o on t.idoras = o.id ' \
                'inner join tari tr on tr.id = o.idTara ' \
                'where tr.id = ' + str(country_id) \
                + ' and ' + 't.timestamp >= ' + from_date \
                + ' and t.timestamp <= ' + until_date + ' order by id asc'
    try:
        results = db_select(query_str)
    except (Exception, psycopg2.DatabaseError) as error:
        print(error)
        return make_response(jsonify([]), 200)
    return make_response(jsonify(results), 200)


if __name__ == '__main__':
    app.run('0.0.0.0', debug=True)
