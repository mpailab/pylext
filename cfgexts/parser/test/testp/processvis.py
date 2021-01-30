import threading
from IPython.display import display
import ipywidgets as widgets
import time

def showProgress(rr):
    if rr.ready:
        print("result is ready:\n"+str(rr.get()))
        return
    elif rr.status=='canceled':
        print("canceled")
        return
    rr._status_widget = widgets.Label('abcd', layout=widgets.Layout(width='100%'))
    rr._progress_widget = widgets.FloatProgress(value=0.0, min=0.0, max=1.0)
    rr._cancel_button = widgets.Button(description='cancel')
    rr._stop_button = widgets.Button(description='stop')
    rr._cancel_button.on_click(lambda _: rr.cancel())
    rr._stop_button.on_click(lambda _: rr.stop())
    rr._hbox = widgets.HBox([rr._progress_widget,rr._stop_button,rr._cancel_button])
    def work(rr):
        total = 100
        st = rr.update()
        while not rr.ready and not rr.canceled:
            rr._progress_widget.value = st.progress
            rr._status_widget.value = st.status
            time.sleep(1)
            st = rr.update()
        #print('finish process')
        rr._hbox.close()
        del rr._hbox
        #rr._progress_widget.close()
        #rr._cancel_button.close()
        if not rr.canceled:
            s = str(rr.get())
            if len(s)>200: s= s[0:200]+'...'
            rr._status_widget.value = "result is ready:\n"+s
        else: 
            rr._status_widget.value = "process canceled"
            print("process canceled")

    rr._display_thread = threading.Thread(target=work, args=(rr,))
    display(rr._status_widget)
    display(rr._hbox)
    #display(rr._cancel_button)
    rr._display_thread.start()