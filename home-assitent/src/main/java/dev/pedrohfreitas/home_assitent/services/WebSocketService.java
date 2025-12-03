package dev.pedrohfreitas.home_assitent.services;

import java.util.Map;
import java.util.concurrent.ConcurrentHashMap;

import org.springframework.stereotype.Component;
import org.springframework.web.socket.TextMessage;
import org.springframework.web.socket.WebSocketMessage;
import org.springframework.web.socket.WebSocketSession;
import org.springframework.web.socket.handler.TextWebSocketHandler;

@Component
public class WebSocketService extends TextWebSocketHandler {
	
	
	public final Map<String, WebSocketSession> sessions = new ConcurrentHashMap<>();
	
	@Override
	public void afterConnectionEstablished(WebSocketSession session) throws Exception {
		String id = session.getUri().getQuery().split("=")[1];
		sessions.put(id, session);
	}
	
	@Override
	public void handleMessage(WebSocketSession session, WebSocketMessage<?> message) throws Exception {
		// TODO Auto-generated method stub
		super.handleMessage(session, message);
	}
	
	@Override
    protected void handleTextMessage(WebSocketSession session, TextMessage message) throws Exception {
        // recebeu mensagem aqui
    }

    public void enviarSomentePara(String id, String msg) throws Exception {
        var s = sessions.get(id);
        if (s != null && s.isOpen()) {
            s.sendMessage(new TextMessage(msg));
        }
    }
	

}
