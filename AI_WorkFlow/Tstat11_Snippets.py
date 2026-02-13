from IPython.display import display, HTML

def main_ui():
    html_code = """
    <div id="tstat-frame" style="width:480px; height:320px; background:#000; border:10px solid #333; border-radius:15px; position:relative; overflow:hidden; font-family:sans-serif; color:white;">
        
        <div style="position:absolute; left:0; top:0; width:60px; height:100%; background:#1a1a1a; display:flex; flex-direction:column; align-items:center; justify-content:space-around; z-index:10; border-right:1px solid #333;">
            <div style="font-size:24px; cursor:pointer; padding:10px;" onclick="switchTstat('home')">🏠</div>
            <div style="font-size:24px; cursor:pointer; padding:10px;" onclick="switchTstat('sched')">📅</div>
            <div style="font-size:24px; cursor:pointer; padding:10px;" onclick="switchTstat('set')">⚙️</div>
        </div>

        <div style="margin-left:60px; height:100%; position:relative;">
            
            <div id="t-home" class="t-screen" style="display:block; text-align:center; padding-top:80px;">
                <div style="font-size:80px; font-weight:bold;">72°</div>
                <div style="color:#00A4FF; font-weight:bold; letter-spacing:2px;">COOLING</div>
            </div>

            <div id="t-sched" class="t-screen" style="display:none; padding:20px;">
                <h2 style="color:#00A4FF; margin-top:0;">Schedule</h2>
                <div style="background:#222; padding:10px; border-radius:5px; margin-bottom:5px;">Work: 68°F (08:00)</div>
                <div style="background:#222; padding:10px; border-radius:5px;">Sleep: 70°F (22:00)</div>
            </div>

            <div id="t-set" class="t-screen" style="display:none; padding:20px;">
                <h2 style="color:#00A4FF; margin-top:0;">Settings</h2>
                <p>IP: 192.168.1.104</p>
                <p>BACnet ID: 12201</p>
                <button style="background:#00A4FF; border:none; color:white; padding:10px; border-radius:5px; width:100%;" onclick="alert('Relay Test Initiated')">Test Relays</button>
            </div>
        </div>
    </div>

    <script>
        function switchTstat(screen) {
            // Hide all screens
            const screens = document.getElementsByClassName('t-screen');
            for (let i = 0; i < screens.length; i++) {
                screens[i].style.display = 'none';
            }
            // Show target
            document.getElementById('t-' + screen).style.display = 'block';
        }
    </script>
    """
    display(HTML(html_code))
