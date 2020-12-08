from flask import Flask  # , url_for
from flask import request
from flask import render_template
from mutantbench import session, db


app = Flask(__name__)


@app.route('/', methods=['GET', 'POST'])
def index():
    if request.method == 'POST':
        pass  # TODO handle post
    return render_template(
        'index.html',
        name='MutantBench',
        operators=session.query(db.Operator).all(),
    )
