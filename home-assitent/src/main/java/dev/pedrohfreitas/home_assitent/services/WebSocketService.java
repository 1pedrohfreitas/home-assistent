package dev.pedrohfreitas.home_assitent.services;

import java.util.HashMap;
import java.util.Map;
import java.util.concurrent.ConcurrentHashMap;

import org.springframework.stereotype.Component;
import org.springframework.web.socket.TextMessage;
import org.springframework.web.socket.WebSocketMessage;
import org.springframework.web.socket.WebSocketSession;
import org.springframework.web.socket.handler.TextWebSocketHandler;

@Component
public class WebSocketService extends TextWebSocketHandler {
	
	
	public final Map<String,Map<String, WebSocketSession>> sessions = new ConcurrentHashMap<>();
	
	@Override
	public void afterConnectionEstablished(WebSocketSession session) throws Exception {
		String[] variaveis = session.getUri().getQuery().split("ws")[0].split("&");
		String userId = null;
		String deviceId = null;
		for(var variavel : variaveis) {
			if(variavel.contains("userId")) {
				userId= variavel.split("=")[1];
			}
			if(variavel.contains("deviceId")) {
				deviceId= variavel.split("=")[1];
			}
		}
		if(userId != null && deviceId != null) {
			if(sessions.containsKey(userId)) {
				sessions.get(userId).put(deviceId, session);
			}
			if(!sessions.containsKey(userId)) {
				Map<String, WebSocketSession> userSession = new HashMap<String, WebSocketSession>();
				userSession.put(deviceId, session);
				sessions.put(userId,userSession);
			}
		}
		
		
		
	}
	
	@Override
	public void handleMessage(WebSocketSession session, WebSocketMessage<?> message) throws Exception {
		// TODO Auto-generated method stub
		super.handleMessage(session, message);
	}
	
	@Override
    protected void handleTextMessage(WebSocketSession session, TextMessage message) throws Exception {
        System.out.println(message);
    }

    public void enviarSomentePara(String userId, String deviceId, String msg) throws Exception {
    	if(sessions.containsKey(userId)){
    		Map<String, WebSocketSession> userSession = sessions.get(userId);
    		
    		if(userSession.containsKey(deviceId) 
        			&& userSession.get(deviceId).isOpen()) {
    			WebSocketSession deviceSession = userSession.get(deviceId);
    			deviceSession.sendMessage(new TextMessage(msg));
    		}
    	}
    }
}
