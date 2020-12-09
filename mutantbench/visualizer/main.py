from flask import Flask  # , url_for
from flask import request
from flask import render_template, send_file
from mutantbench import session, db
from io import BytesIO
from matplotlib import pyplot as plt


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


@app.route('/fig/<operators>/<equivalency>')
def fig(operators, equivalency):
    plt.plot([1, 2, 3, 4], [1, 2, 3, 4])
    img = BytesIO()
    plt.savefig(img)
    img.seek(0)
    return send_file(img, mimetype='image/png')
