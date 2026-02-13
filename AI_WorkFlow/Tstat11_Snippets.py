# Tstat11 UI Library
from IPython.display import display, HTML

def main_ui():
    html_code = """
    <div id='device-bezel' style='width:480px; height:320px; background:black; border:12px solid #333; border-radius:15px; position:relative; overflow:hidden; font-family:sans-serif;'>
        <div style='text-align:center; margin-top:100px;'>
            <div style='font-size:72px; font-weight:bold; color:white;'>72.5°F</div>
            <div style='color:#00A4FF; font-size:16px; font-weight:bold;'>COOLING • AUTO</div>
        </div>
        <div style='position:absolute; bottom:15px; width:100%; text-align:center; color:#555; font-size:12px;'>TEMCO CONTROLS • TSTAT11</div>
    </div>
    """
    display(HTML(html_code))
