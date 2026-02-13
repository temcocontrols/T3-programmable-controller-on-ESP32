from IPython.display import display, HTML

def main_ui():
    html_code = """
    <div id="tstat-container" style="width:480px; height:320px; background:black; border:12px solid #333; border-radius:15px; position:relative; overflow:hidden; font-family:sans-serif; user-select:none;">
        
        <div style="position:absolute; top:12px; left:15px; font-size:28px; cursor:pointer; z-index:100;" onclick="showScreen('settings')">⚙️</div>
        <div style="position:absolute; top:12px; right:15px; font-size:28px; cursor:pointer; z-index:100;" onclick="showScreen('schedule')">📅</div>
        <div style="position:absolute; bottom:12px; left:15px; font-size:28px; cursor:pointer; z-index:100;" onclick="showScreen('home')">🏠</div>

        <div id="screen-home" class="screen" style="text-align:center; margin-top:100px;">
            <div style="font-size:72px; font-weight:bold; color:white;">72.5°F</div>
            <div style="color:#00A4FF; font-size:16px; font-weight:bold; margin-top:5px;">COOLING • AUTO</div>
        </div>

        <div id="screen-settings" class="screen" style="display:none; padding:40px; color:white;">
            <h3 style="margin-top:0; color:#00A4FF;">Service Menu</h3>
            <div style="display:flex; flex-direction:column; gap:10px;">
                <div style="background:#222; padding:10px; border-radius:5px;">Network: BACnet/IP</div>
                <div style="background:#222; padding:10px; border-radius:5px;">Instance: 1001</div>
                <div style="background:#00A4FF; padding:10px; border-radius:5px; text-align:center; cursor:pointer;" onclick="alert('Searching for Aqara...')">Pair Matter Device</div>
            </div>
        </div>

        <div id="screen-schedule" class="screen" style="display:none; padding:40px; color:white;">
            <h3 style="margin-top:0; color:#00A4FF;">Weekly Schedule</h3>
            <div style="font-size:14px;">
                <p>MON-FRI: 68°F (8am) | 72°F (5pm)</p>
                <p>SAT-SUN: 70°F (All Day)</p>
            </div>
        </div>

    </div>

    <script>
        function showScreen(screenId) {
            // Hide all screens
            document.querySelectorAll('.screen').forEach(s => s.style.display = 'none');
            // Show the selected one
            document.getElementById('screen-' + screenId).style.display = 'block';
        }
    </script>
    """
    display(HTML(html_code))
