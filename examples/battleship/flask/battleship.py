from flask import Flask, escape, request, redirect
from flask import render_template
import json
import os
import random
app = Flask(__name__)

@app.route('/')
def hello():
    return redirect("battleship")

@app.route('/battleship', methods=['POST', 'GET'])
def battleship():
    userinput = ""
    playerships = ""
    battleshipresponse = ""
    if request.method == 'GET':
        operation = request.args.get('operation', default = -1, type = str)
        if operation == "shoot":
            token = request.args.get('token', default = -1, type = str)
            x = request.args.get('shootx', default = -1, type = int)
            y = request.args.get('shooty', default = -1, type = int)
            if x != -1 and y != -1:
                userinput = token;
                filename = os.path.join(app.instance_path, 'shoot.txt'+str(random.randint(1, 10001)))
                cmd='cd ../../../integration; ./battleship_invoke.sh Shoot \'{\\\"x\\\":'+str(x)+',\\\"y\\\":'+str(y)+',\\\"token_string\\\":\\\"'+userinput+'\\\"}\' '+filename+'; cd ../examples/battleship/flask'
                print cmd
                if os.system(cmd):
                    raise RuntimeError('program failed!')
                shootfile = open(filename, 'r+')
                content = shootfile.read()
                shootfile.close()
                os.remove(filename)
                battleshipresponse = content

        if operation == "refresh":
            userinput = request.args.get('token', default = -1, type = str)
            filename = os.path.join(app.instance_path, 'refresh.txt'+str(random.randint(1, 10001)))
            #cmd='cd ../../../integration; ./battleship_invoke.sh RefreshPlayer \'{\\\"token_string\\\":\\\"'+userinput+'\\\"}\' '+filename+'; cd ../examples/battleship/flask'
            cmd='cd ../../../integration; ./battleship_query.sh RefreshPlayer \'{\\\"token_string\\\":\\\"'+userinput+'\\\"}\' '+filename+'; cd ../examples/battleship/flask'
            print cmd
            if os.system(cmd):
                raise RuntimeError('program failed!')
            refreshfile = open(filename, 'r+')
            refresh_content = refreshfile.read()
            refreshfile.close()
            os.remove(filename)
            battleshipresponse = refresh_content

    if request.method == 'POST':
        print request
        print request.form
        print request.data

        if "new game" in request.form:
            filename = os.path.join(app.instance_path, 'refresh.txt'+str(random.randint(1, 10001)))
            cmd='cd ../../../integration; ./battleship_invoke.sh NewGame \'\' '+filename+'; cd ../examples/battleship/flask'
            print cmd
            if os.system(cmd):
                raise RuntimeError('program failed!')
            refreshfile = open(filename, 'r+')
            refresh_content = refreshfile.read()
            refreshfile.close()
            os.remove(filename)
            battleshipresponse = refresh_content

        while "ship deployment" in request.form:
            playerships = str(request.form['ship deployment'])
            playerships = playerships.replace(" ", "")
            playerships_arr = map(int, request.form['ship deployment'].split(","))
            battleshipresponse = "deployment successful"
            deployment_successful = True
            #check array size
            if len(playerships_arr) != 20:
                battleshipresponse = "wrong array size"
                deployment_successful = False
                break
            #check ships
            deployable_lengths = [0,0,1,2,1,1];
            for ship in range(5):
                length = playerships_arr[ship*4+0]
                x=playerships_arr[ship*4+1]
                y=playerships_arr[ship*4+2]
                orientation=playerships_arr[ship*4+3]
                if length < 2 or length > 5:
                    battleshipresponse = "wrong length for ship " + str(ship)
                    deployment_successful = False
                    break
                if deployable_lengths[length] == 0:
                    battleshipresponse = "cannot deploy also ship " + str(ship)
                    deployment_successful = False
                    break
                deployable_lengths[length] -= 1
                if x<0 or x>10:
                    battleshipresponse = "wrong x for ship " + str(ship)
                    deployment_successful = False
                    break
                if y<0 or y>10:
                    battleshipresponse = "wrong y for ship " + str(ship)
                    deployment_successful = False
                    break
                if orientation<0 or orientation>1:
                    battleshipresponse = "wrong orientation for ship " + str(ship)
                    deployment_successful = False
                    break          
        
            if deployment_successful:
                #do join
                filename = os.path.join(app.instance_path, 'refresh.txt'+str(random.randint(1, 10001)))
                cmd='cd ../../../integration; ./battleship_invoke.sh JoinGame \'{\\\"ship_array\\\":['+playerships+']}\' '+filename+'; cd ../examples/battleship/flask'
                print cmd
                if os.system(cmd):
                    raise RuntimeError('program failed!')
                refreshfile = open(filename, 'r+')
                refresh_content = refreshfile.read()
                refreshfile.close()
                os.remove(filename)
                battleshipresponse = refresh_content
                j = json.loads(battleshipresponse)
                userinput = j['assigned_token_string']
            break

        if "refresh token" in request.form:
            userinput = request.form['refresh token']
        
            filename = os.path.join(app.instance_path, 'refresh.txt'+str(random.randint(1, 10001)))
            #cmd='cd ../../../integration; ./battleship_invoke.sh RefreshPlayer \'{\\\"token_string\\\":\\\"'+userinput+'\\\"}\' '+filename+'; cd ../examples/battleship/flask'
            cmd='cd ../../../integration; ./battleship_query.sh RefreshPlayer \'{\\\"token_string\\\":\\\"'+userinput+'\\\"}\' '+filename+'; cd ../examples/battleship/flask'
            print cmd
            if os.system(cmd):
                raise RuntimeError('program failed!')
            refreshfile = open(filename, 'r+')
            refresh_content = refreshfile.read()
            refreshfile.close()
            os.remove(filename)
            battleshipresponse = refresh_content

        if ("shoot x" in request.form) and ("shoot y" in request.form):
            userinput=request.form['shoot token']
            x = request.form['shoot x']
            y = request.form['shoot y']
            filename = os.path.join(app.instance_path, 'shoot.txt'+str(random.randint(1, 10001)))
            cmd='cd ../../../integration; ./battleship_invoke.sh Shoot \'{\\\"x\\\":'+x+',\\\"y\\\":'+y+',\\\"token_string\\\":\\\"'+userinput+'\\\"}\' '+filename+'; cd ../examples/battleship/flask'
            print cmd
            if os.system(cmd):
                raise RuntimeError('program failed!')
            shootfile = open(filename, 'r+')
            content = shootfile.read()
            shootfile.close()
            os.remove(filename)
            battleshipresponse = content
            #battleshipresponse = 'shooting x='+x+' y='+y+' auth='+userinput

    #return render_template('my-form.html', text=userinput)
    return render_template('battleship.html', text=userinput, battleshipresponse=battleshipresponse, playerships=playerships)
